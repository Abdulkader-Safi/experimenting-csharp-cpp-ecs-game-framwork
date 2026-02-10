# Web Export Plan

This document outlines the strategy for compiling the C++/C# Vulkan glTF viewer to run in a web browser via WebAssembly.

## Current Architecture

```
C# (Mono)                    C++ (native/)
  Viewer.cs ──main loop──>     bridge.cpp (extern "C")
  ECS/World.cs                 renderer.h / renderer.cpp
  ECS/Systems.cs                 └─ VulkanRenderer class
  ECS/Components.cs                  ├─ GLFW (window/input)
  ECS/NativeBridge.cs                ├─ Vulkan (rendering)
                                     ├─ GLM (math)
                                     └─ cgltf (model loading)
```

**Key dependencies that do NOT work in browsers:**
| Dependency | Problem | Web Replacement |
|---|---|---|
| Vulkan | No browser support | WebGPU (`emscripten/html5_webgpu.h`) |
| GLFW | No native windows in browser | Emscripten HTML5 API (`emscripten/html5.h`) |
| Mono P/Invoke | Mono runtime not in WASM | Port game logic to C++ or use .NET WASM |
| File I/O (`std::ifstream`) | No filesystem | Emscripten virtual FS / `fetch()` preloading |
| `glfwGetTime()` | GLFW unavailable | `emscripten_get_now()` |

## Strategy: Two-Phase Approach

### Phase 1 — C++ Only WebGPU Build (Recommended First)

Port the C++ renderer to WebGPU, move the game loop into C++, compile with `em++`. This is the fastest path to a working browser demo.

### Phase 2 — Restore C# via .NET WASM (Optional)

Use `dotnet workload install wasm-tools` to compile C# ECS logic to WASM, calling into the C++ WASM module. This restores the original C#→C++ architecture inside the browser.

---

## Phase 1: C++ WebGPU Port (Detailed Steps)

### Step 1 — Abstract the Rendering Backend

Create a backend interface so Vulkan and WebGPU can coexist. The native desktop build keeps Vulkan; the Emscripten build uses WebGPU.

**New files:**

```
native/
  backend.h              ← Abstract interface (init, render, cleanup, etc.)
  backend_vulkan.cpp     ← Current VulkanRenderer, adapted to implement backend.h
  backend_webgpu.cpp     ← New WebGPU implementation
  platform.h             ← Window/input abstraction
  platform_glfw.cpp      ← Current GLFW code extracted
  platform_web.cpp       ← Emscripten HTML5 input/canvas
  game.h / game.cpp      ← Game loop + ECS logic ported from C#
```

**backend.h sketch:**

```cpp
#pragma once
#include <glm/glm.hpp>

class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    virtual bool init(int width, int height, const char* title) = 0;
    virtual void cleanup() = 0;
    virtual int loadMesh(const char* path) = 0;
    virtual int createEntity(int meshId) = 0;
    virtual void setEntityTransform(int entityId, const float* mat4x4) = 0;
    virtual void removeEntity(int entityId) = 0;
    virtual void setCamera(float eyeX, float eyeY, float eyeZ,
                           float targetX, float targetY, float targetZ,
                           float upX, float upY, float upZ, float fovDegrees) = 0;
    virtual void setLight(int index, int type, /* ... same params ... */) = 0;
    virtual void clearLight(int index) = 0;
    virtual void setAmbientIntensity(float intensity) = 0;
    virtual void renderFrame() = 0;
};
```

The `bridge.cpp` extern "C" functions remain unchanged — they just dispatch to whichever backend is active (selected at compile time via `#ifdef __EMSCRIPTEN__`).

### Step 2 — Cross-Compile Shaders to WGSL

WebGPU uses WGSL, not GLSL/SPIR-V. Use the toolchain you already have:

```bash
# GLSL → SPIR-V (already done)
glslc native/shaders/shader.vert -o build/shaders/vert.spv
glslc native/shaders/shader.frag -o build/shaders/frag.spv

# SPIR-V → WGSL (new step)
spirv-cross --wgsl build/shaders/vert.spv --output build/shaders/shader.wgsl.vert
spirv-cross --wgsl build/shaders/frag.spv --output build/shaders/shader.wgsl.frag
```

**Known issues to handle during conversion:**

- Push constants don't exist in WebGPU — convert to a uniform/storage buffer at bind group 1
- `std140` layout in UBOs maps to WGSL `uniform` with explicit `@size`/`@align`
- `gl_Position.y` may need flipping depending on coordinate convention
- Matrix inverse in vertex shader (`transpose(inverse(model))`) — verify WGSL support or precompute normal matrix on CPU

**Add to Makefile:**

```makefile
WGSL_VERT = $(SHADER_DIR)/shader.wgsl.vert
WGSL_FRAG = $(SHADER_DIR)/shader.wgsl.frag

$(WGSL_VERT): $(VERT_SPV)
	spirv-cross --wgsl $< --output $@

$(WGSL_FRAG): $(FRAG_SPV)
	spirv-cross --wgsl $< --output $@

wgsl-shaders: $(WGSL_VERT) $(WGSL_FRAG)
```

### Step 3 — Implement WebGPU Backend (`backend_webgpu.cpp`)

Replace each Vulkan concept with its WebGPU equivalent:

| Vulkan Concept                    | WebGPU Equivalent                                |
| --------------------------------- | ------------------------------------------------ |
| `VkInstance`                      | `wgpuCreateInstance()`                           |
| `VkDevice`                        | `WGPUDevice` via `wgpuAdapterRequestDevice()`    |
| `VkSwapchainKHR`                  | `WGPUSurface` + `wgpuSurfaceGetCurrentTexture()` |
| `VkRenderPass`                    | `WGPURenderPassEncoder` (created per-frame)      |
| `VkPipeline`                      | `WGPURenderPipeline`                             |
| `VkBuffer` (vertex/index/uniform) | `WGPUBuffer`                                     |
| `VkDescriptorSet`                 | `WGPUBindGroup`                                  |
| Push constants                    | Uniform buffer in bind group 1                   |
| `VkCommandBuffer`                 | `WGPUCommandEncoder` → `WGPUCommandBuffer`       |
| `vkQueueSubmit`                   | `wgpuQueueSubmit()`                              |
| `VkShaderModule` (SPIR-V)         | `WGPUShaderModule` (WGSL)                        |
| Depth image                       | `WGPUTexture` with depth format                  |
| `VkSemaphore`/`VkFence`           | Not needed (WebGPU handles sync)                 |

**Emscripten WebGPU setup:**

```cpp
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu.h>

// Get the WebGPU device Emscripten already negotiated
WGPUDevice device = emscripten_webgpu_get_device();
```

### Step 4 — Replace GLFW with Emscripten HTML5 APIs

| GLFW Function                       | Emscripten Replacement                                  |
| ----------------------------------- | ------------------------------------------------------- |
| `glfwInit()` + `glfwCreateWindow()` | HTML `<canvas>` element (exists in the generated .html) |
| `glfwPollEvents()`                  | Not needed (browser event loop)                         |
| `glfwGetKey()`                      | `emscripten_set_keydown_callback()` + key state array   |
| `glfwGetCursorPos()`                | `emscripten_set_mousemove_callback()`                   |
| `glfwSetInputMode(CURSOR_DISABLED)` | `emscripten_request_pointerlock()`                      |
| `glfwWindowShouldClose()`           | Always false (browser tab handles close)                |
| `glfwGetTime()`                     | `emscripten_get_now() / 1000.0`                         |
| `glfwGetFramebufferSize()`          | `emscripten_get_canvas_element_size()`                  |

### Step 5 — Port Game Logic from C# to C++

The ECS code in `managed/ecs/` is small (~200 lines total). Port it to C++:

```
native/
  game/
    world.h / world.cpp        ← from managed/ecs/World.cs
    components.h               ← from managed/ecs/Components.cs
    systems.h / systems.cpp    ← from managed/ecs/Systems.cs
```

The C# ECS is simple enough that this is straightforward:

- `World` is a container of entities with component storage and system dispatch
- `Components` are plain data structs (Transform, MeshComponent, Movable, Camera, Light)
- `Systems` are free functions that query the world and call renderer APIs

### Step 6 — Emscripten Main Loop

Browsers don't support `while(true)` loops. Replace with:

```cpp
// Current (blocking):
while (!renderer_should_close()) {
    pollEvents();
    world.runSystems();
    renderFrame();
}

// Emscripten (callback-driven):
void mainLoopIteration() {
    world.updateTime();
    world.runSystems();
    backend->renderFrame();
}

int main() {
    // ... init ...
    emscripten_set_main_loop(mainLoopIteration, 0, true);
    // 0 = use requestAnimationFrame, true = simulate infinite loop
}
```

### Step 7 — Asset Loading

Browser has no filesystem. Two options:

**Option A — Preload with `--preload-file` (simplest):**

```bash
em++ ... --preload-file models/AnimationLibrary_Godot_Standard.glb@/models/model.glb \
         --preload-file build/shaders/shader.wgsl.vert@/shaders/shader.wgsl.vert \
         --preload-file build/shaders/shader.wgsl.frag@/shaders/shader.wgsl.frag
```

Files are packed into a `.data` file and available via normal `fopen`/`ifstream` in Emscripten's virtual filesystem. **No code changes needed for file I/O.**

**Option B — Async fetch (for larger assets):**

Use `emscripten_async_wget()` or the Fetch API. More complex but avoids large upfront downloads.

**Recommendation:** Start with Option A. It requires zero changes to `cgltf` or shader loading code.

### Step 8 — Build System

**New Makefile targets:**

```makefile
# --- Web build ---
WEB_DIR = build/web
WASM_OUT = $(WEB_DIR)/viewer.html

web: wgsl-shaders
	mkdir -p $(WEB_DIR)
	em++ -O2 \
		-s USE_WEBGPU=1 \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
		-I native/vendor \
		-I /opt/homebrew/include \
		--preload-file models/@/models/ \
		--preload-file build/shaders/@/shaders/ \
		native/renderer_webgpu.cpp \
		native/bridge.cpp \
		native/game/world.cpp \
		native/game/systems.cpp \
		native/main_web.cpp \
		-o $(WASM_OUT)

serve-web: web
	cd $(WEB_DIR) && python3 -m http.server 8080
```

**Key em++ flags:**

- `-s USE_WEBGPU=1` — enables WebGPU bindings
- `-s ALLOW_MEMORY_GROWTH=1` — heap can grow (needed for model loading)
- `--preload-file` — bundles assets into virtual FS
- `-O2` — optimize for size/speed

### Step 9 — Test in Browser

```bash
make web
make serve-web
# Open http://localhost:8080/viewer.html in Chrome/Edge (WebGPU required)
```

**Browser requirements:**

- Chrome 113+ or Edge 113+ (WebGPU enabled by default)
- Firefox Nightly (behind `dom.webgpu.enabled` flag)
- Safari 18+ (partial WebGPU support)

---

## Phase 2: Restore C# via .NET WASM (Optional)

If you want C# driving the game logic in the browser (preserving the P/Invoke architecture):

### Step 1 — Install .NET WASM Workload

```bash
dotnet workload install wasm-tools
dotnet workload install wasm-experimental  # for browser interop
```

### Step 2 — Create a Blazor WASM or Console WASM Project

```bash
dotnet new wasmbrowser -o web/managed
```

### Step 3 — Interop Pattern

Instead of P/Invoke to a `.dylib`, use `[DllImport("__internal")]` which maps to WASM imports. The C++ module (compiled separately with `em++`) exports its functions, and the .NET WASM runtime imports them.

```
.NET WASM Runtime (C# ECS)
  └─ [DllImport] ──JS interop──> C++ WASM module (renderer)
```

This is significantly more complex than Phase 1. The .NET WASM runtime adds ~5-10 MB to the download. Recommended only if you need C# hot-reload or plan to scale the game logic significantly.

---

## File Changes Summary

### New Files (Phase 1)

| File                        | Purpose                                                |
| --------------------------- | ------------------------------------------------------ |
| `native/backend.h`          | Abstract rendering interface                           |
| `native/backend_vulkan.cpp` | Vulkan impl (extract from `renderer.cpp`)              |
| `native/backend_webgpu.cpp` | WebGPU impl (new)                                      |
| `native/platform.h`         | Window/input abstraction                               |
| `native/platform_glfw.cpp`  | GLFW impl (extract from `renderer.cpp`)                |
| `native/platform_web.cpp`   | Emscripten HTML5 input (new)                           |
| `native/game/world.h`       | C++ port of `managed/ecs/World.cs`                     |
| `native/game/world.cpp`     | C++ port of `managed/ecs/World.cs`                     |
| `native/game/components.h`  | C++ port of `managed/ecs/Components.cs`                |
| `native/game/systems.h`     | C++ port of `managed/ecs/Systems.cs`                   |
| `native/game/systems.cpp`   | C++ port of `managed/ecs/Systems.cs`                   |
| `native/main_web.cpp`       | Emscripten entry point with `emscripten_set_main_loop` |
| `build/shaders/*.wgsl.*`    | Cross-compiled WGSL shaders (generated)                |

### Modified Files

| File                    | Change                                                    |
| ----------------------- | --------------------------------------------------------- |
| `native/renderer.cpp`   | Extract platform code; add `#ifdef __EMSCRIPTEN__` guards |
| `native/renderer.h`     | Refactor into `backend.h` interface                       |
| `native/bridge.cpp`     | Dispatch to active backend via compile-time flag          |
| `native/CMakeLists.txt` | Add Emscripten toolchain support and WebGPU target        |
| `Makefile`              | Add `web`, `wgsl-shaders`, `serve-web` targets            |

### Unchanged Files

| File                         | Why                                                          |
| ---------------------------- | ------------------------------------------------------------ |
| `managed/**/*.cs`            | Still used for native desktop build; web build uses C++ port |
| `native/shaders/shader.vert` | Source of truth; WGSL generated from SPIR-V output           |
| `native/shaders/shader.frag` | Source of truth; WGSL generated from SPIR-V output           |
| `native/vendor/cgltf.h`      | Works unchanged under Emscripten with `--preload-file`       |

---

## Pre-Flight Checklist

Run this before starting implementation to verify tools are ready:

```bash
# Emscripten
emcc --version          # expect 4.x+
em++ --version

# Shader cross-compilation
glslc --version
spirv-cross --version

# Test WGSL conversion
glslc native/shaders/shader.vert -o /tmp/test.spv
spirv-cross --wgsl /tmp/test.spv --output /tmp/test.wgsl
cat /tmp/test.wgsl      # verify WGSL output looks correct

# Test basic em++ compile
echo '#include <emscripten.h>
void main_loop() {}
int main() { emscripten_set_main_loop(main_loop, 0, 1); }' > /tmp/em_test.cpp
em++ -o /tmp/em_test.html /tmp/em_test.cpp && echo "OK"

# Test WebGPU compile
echo '#include <webgpu/webgpu.h>
int main() { WGPUInstanceDescriptor desc = {}; return 0; }' > /tmp/wgpu_test.cpp
em++ -s USE_WEBGPU=1 -o /tmp/wgpu_test.html /tmp/wgpu_test.cpp && echo "WebGPU OK"

# Cleanup
rm -f /tmp/test.spv /tmp/test.wgsl /tmp/em_test.* /tmp/wgpu_test.*
```

## Implementation Order

1. Run pre-flight checklist (verify WGSL conversion works)
2. Add `wgsl-shaders` Makefile target, verify shader output
3. Create `backend.h` interface
4. Extract `backend_vulkan.cpp` and `platform_glfw.cpp` from `renderer.cpp` (desktop still works)
5. Port C# ECS to `native/game/` (desktop still works via C#)
6. Implement `platform_web.cpp` (Emscripten input)
7. Implement `backend_webgpu.cpp` (the big one)
8. Create `main_web.cpp` with `emscripten_set_main_loop`
9. Add `web` / `serve-web` Makefile targets
10. Test in Chrome, iterate on WebGPU bugs
11. Optimize: `-O2`, texture compression, asset streaming

## Risks and Mitigations

| Risk                                         | Impact                 | Mitigation                                                 |
| -------------------------------------------- | ---------------------- | ---------------------------------------------------------- |
| `spirv-cross --wgsl` fails on push constants | Shaders won't compile  | Convert push constants to UBO before cross-compiling       |
| WebGPU API differences from Vulkan           | Rendering bugs         | Use Dawn's reference examples; test incrementally          |
| Large `.glb` model download time             | Slow first load        | Compress with Draco; show loading progress bar             |
| Browser WebGPU support gaps                  | Doesn't run everywhere | Provide WebGL2 fallback or "browser not supported" message |
| cgltf under Emscripten                       | May have issues        | cgltf is header-only C, known to work with Emscripten      |

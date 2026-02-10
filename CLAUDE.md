# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make viewer          # Build shaders + C++ dylib + C# exe
make run             # Build and run the Vulkan viewer
make app             # Build macOS .app bundle
make shaders         # Compile GLSL → SPIR-V only
make clean           # Remove all build artifacts
make all             # Build hello demo (basic P/Invoke test)
```

The viewer build does three things in sequence:
1. `glslc` compiles `.vert`/`.frag` → `.spv` in `build/shaders/`
2. CMake builds `native/` → `build/librenderer.dylib`
3. `mcs` compiles all `managed/**/*.cs` → `build/Viewer.exe`

`make run` sets `DYLD_LIBRARY_PATH` and `VK_ICD_FILENAMES` (MoltenVK) automatically.

## Documentation Site

```bash
cd docs && bun install && bun run start   # Dev server at localhost:3000
cd docs && bun run build                  # Production build (validates broken links)
```

Uses Docusaurus 3.9.2 with `docusaurus-plugin-auto-sidebars` and `docusaurus-lunr-search`.

## Architecture

C# (Mono) drives the main loop and ECS game logic. C++ handles Vulkan rendering. They communicate via P/Invoke through a shared library.

```
C# World (managed/ecs/)          C++ VulkanRenderer (native/)
  Entities + Components   ──P/Invoke──>  GPU resources
  Systems (per-frame)     ──────────>    Draw calls
  NativeBridge.cs         ──DllImport──> bridge.cpp (extern "C")
```

**Adding a new native function requires changes in three places:**
1. `native/bridge.cpp` — `extern "C"` wrapper calling into `VulkanRenderer`
2. `native/renderer.h` + `renderer.cpp` — implementation on the C++ class
3. `managed/ecs/NativeBridge.cs` — `[DllImport("renderer")]` declaration

**Adding a new C# file:** add it to the `VIEWER_CS` list in the `Makefile`.

## ECS Pattern

- **Components** are plain C# classes (data only, no behavior, must be reference types)
- **Systems** are `static void SystemName(World world)` methods that query and mutate
- **System order matters:** register input systems first, `RenderSyncSystem` always last
- `world.DeltaTime` is set each frame from `NativeBridge.renderer_get_delta_time()`

Built-in system chain: `InputMovementSystem` → `CameraFollowSystem` → `LightSyncSystem` → `RenderSyncSystem`

## Key Constraints

- MoltenVK ICD path: `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` (not under `/share/`)
- MoltenVK requires `VK_KHR_portability_enumeration` instance extension + `VK_KHR_portability_subset` device extension
- `cgltf.h` needs `#define CGLTF_IMPLEMENTATION` in exactly one `.cpp` file
- `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` ≠ `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT` — different enum types
- GLFW/Vulkan tests fail in headless/subprocess contexts (no window server) — expected on macOS
- Fragment shader supports max 8 dynamic lights (defined as `MAX_LIGHTS` in shader and C++)
- Push constants carry per-entity model matrix; UBOs carry view/proj and light data

# Architecture Overview

This page describes the high-level architecture of the Safi Engine: how C# game logic communicates with the C++ Vulkan renderer, what each file is responsible for, and how the build pipeline produces a running application.

## High-Level Architecture

The engine is split into two language domains. C# (running on Mono) owns the main loop, ECS world, and all gameplay logic. C++ owns the Vulkan rendering backend. The two sides communicate exclusively through P/Invoke -- a foreign-function interface where C# calls `extern "C"` functions exported from a shared library.

```
C# Engine (managed/)            C++ VulkanRenderer (native/)
  World, Components, Bridge
C# Game (game_logic/)
  Scene setup, Systems       ──P/Invoke──>  GPU resources
  Per-frame logic            ──────────>    Draw calls
  NativeBridge.cs            ──DllImport──> bridge.cpp (extern "C")
```

Data flows **one direction**: C# tells C++ what to render. The native side never calls back into managed code. Every frame, C# systems iterate over ECS components and issue bridge calls to set transforms, update lights, submit UI vertices, and trigger the draw. The C++ side translates these into Vulkan command buffers.

A separate P/Invoke boundary exists for physics: `PhysicsBridge.cs` calls into `libjoltc.dylib` (built from the `native/joltc/` submodule). The `PhysicsWorld` singleton manages the Jolt lifecycle, and `PhysicsSystem` (in game logic) creates bodies, steps the simulation, and syncs transforms back to ECS.

## File Map

| File                          | Purpose                                                                                                                                                                                                                                                                                                        |
| ----------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `native/renderer.h`           | `VulkanRenderer` class declaration and all GPU-facing struct definitions (`Vertex`, `UIVertex`, `GpuLight`, `LightUBO`, etc.). Also defines `MAX_LIGHTS` (8) and light type constants.                                                                                                                         |
| `native/renderer.cpp`         | ~3000 lines. The entire Vulkan implementation: instance/device/swapchain creation, render pass setup, both graphics pipelines, mesh loading (glTF via cgltf), texture loading (stb_image), font atlas baking (stb_truetype), UI vertex buffer management, lighting UBO updates, and the per-frame render loop. |
| `native/bridge.cpp`           | `extern "C"` wrappers that delegate to a file-static `g_renderer` instance of `VulkanRenderer`. Each function is a thin try/catch shell around the corresponding method. This is the only translation unit that C# can see.                                                                                    |
| `native/shaders/shader.vert`  | 3D vertex shader. Reads a UBO containing `view` and `proj` matrices, receives the per-entity `model` matrix via push constant. Outputs world-space position, normal, vertex color, and UV coordinates to the fragment stage.                                                                                   |
| `native/shaders/shader.frag`  | 3D fragment shader. Implements Blinn-Phong shading with support for up to 8 dynamic lights (directional, point, spot). Samples a base color texture and combines it with per-vertex color and lighting.                                                                                                        |
| `native/shaders/ui.vert`      | UI vertex shader. Converts pixel coordinates to NDC using a `screenSize` vec2 push constant. Passes through UV and vertex color.                                                                                                                                                                               |
| `native/shaders/ui.frag`      | UI fragment shader. Samples an `R8_UNORM` font atlas texture, multiplies the single-channel alpha by the vertex color, and outputs for alpha blending.                                                                                                                                                         |
| `native/CMakeLists.txt`       | CMake configuration. Links against Vulkan, GLFW, and GLM. Produces `librenderer.dylib`. Enables `VK_KHR_portability_enumeration` for MoltenVK compatibility. Generates `compile_commands.json` for IDE intellisense.                                                                                           |
| `native/vendor/`              | Header-only third-party libraries: `cgltf.h` (glTF 2.0 parsing), `stb_truetype.h` (TrueType font rasterization), `stb_image.h` (image decoding for textures). Each requires a `#define *_IMPLEMENTATION` in exactly one `.cpp` file.                                                                           |
| `managed/Viewer.cs`           | Application entry point. Creates the ECS `World`, calls `Game.Setup(world)`, and runs the main loop.                                                                                                                                                                                                           |
| `managed/World.cs`            | ECS world: entity creation/despawning, component storage, and `Query()` for matching entities by component type.                                                                                                                                                                                               |
| `managed/Components.cs`       | All component definitions (plain C# classes, data only). Includes `Color` (RGBA with hex string support), `Transform`, `MeshRenderer`, `Camera`, `Light`, `Hierarchy`, `Rigidbody`, `Collider` (with `DebugColor` for wireframe visualization), etc.                                                           |
| `managed/NativeBridge.cs`     | C# P/Invoke declarations (`[DllImport("renderer")]`) for every function exported by `bridge.cpp`. Also defines GLFW key/mouse constants used by input systems.                                                                                                                                                 |
| `managed/PhysicsBridge.cs`    | C# P/Invoke declarations (`[DllImport("joltc")]`) for the Jolt Physics C API. Defines blittable structs (`JPH_Vec3`, `JPH_RVec3`, `JPH_Quat`, `JPH_Plane`, `JPH_PhysicsSystemSettings`) and enums (`JPH_MotionType`, `JPH_Activation`).                                                                        |
| `managed/PhysicsWorld.cs`     | Singleton managing the Jolt Physics lifecycle: init, fixed-timestep stepping, body creation/removal, position/rotation readback, and shutdown. Survives hot reloads (engine layer).                                                                                                                            |
| `managed/FreeCameraState.cs`  | Static state for the debug free camera.                                                                                                                                                                                                                                                                        |
| `managed/HotReload.cs`        | File watcher + recompiler for dev mode. Watches `game_logic/` for changes, recompiles `GameLogic.dll`, then resets the world and re-runs `Game.Setup` for full scene re-initialization.                                                                                                                        |
| `game_logic/Game.cs`          | Scene setup entry point. `Game.Setup(world)` spawns entities, configures components, and registers systems.                                                                                                                                                                                                    |
| `game_logic/Systems.cs`       | All system implementations (`static void` methods). The built-in system chain runs in registration order: InputMovement, Timer, Physics, FreeCamera, CameraFollow, LightSync, HierarchyTransform, DebugOverlay, DebugColliderRender, RenderSync.                                                               |
| `game_logic/GameConstants.cs` | Tunable config values (debug mode, camera sensitivity, movement speed).                                                                                                                                                                                                                                        |
| `Makefile`                    | Top-level build orchestration: shader compilation, CMake invocation, C# compilation, and the `run` target that sets environment variables and launches Mono.                                                                                                                                                   |
| `SaFiEngine.sln`              | Solution file at repo root. Used by IDEs (VS Code C# Dev Kit, OmniSharp) to discover the C# project. Not used by the Makefile build.                                                                                                                                                                           |
| `managed/SaFiEngine.csproj`   | .NET project file for IDE IntelliSense (autocomplete, go-to-definition). Targets `net10.0`. Not used by the Makefile build — `mcs` compiles directly from `VIEWER_CS`.                                                                                                                                         |

## Build Pipeline

Running `make run` executes four steps in sequence:

### 1. Shader Compilation

```
glslc native/shaders/shader.vert  → build/shaders/vert.spv
glslc native/shaders/shader.frag  → build/shaders/frag.spv
glslc native/shaders/ui.vert      → build/shaders/ui_vert.spv
glslc native/shaders/ui.frag      → build/shaders/ui_frag.spv
```

Each `.vert` and `.frag` file is compiled from GLSL to SPIR-V using Google's `glslc` compiler. The SPIR-V binaries land in `build/shaders/` where the renderer loads them at init.

### 2. Native Library Build

```
cmake -S native -B build/native -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/native
```

CMake configures and builds the C++ code into `build/librenderer.dylib`. A symlink to `compile_commands.json` is placed at the repo root for IDE support.

### 3. C# Compilation

```
mcs -out:build/Viewer.exe managed/Viewer.cs managed/World.cs \
    managed/Components.cs managed/NativeBridge.cs \
    managed/FreeCameraState.cs \
    game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
```

All managed source files listed in the Makefile's `MANAGED_CS` and `GAMELOGIC_CS_FILES` variables are compiled into a single Mono executable.

### 4. Run

```bash
DYLD_LIBRARY_PATH=build:/opt/homebrew/lib \
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
    mono build/Viewer.exe
```

`DYLD_LIBRARY_PATH` tells Mono where to find `librenderer.dylib` (and GLFW/MoltenVK from Homebrew). `VK_ICD_FILENAMES` points Vulkan's loader to the MoltenVK ICD so that Vulkan runs on top of Metal.

## Triple-Pipeline Architecture

The renderer runs **three graphics pipelines** within a single Vulkan render pass. All pipelines share the same swapchain framebuffers and command buffers, but they have different pipeline states.

### 3D Scene Pipeline

| Setting        | Value                                                                      |
| -------------- | -------------------------------------------------------------------------- |
| Depth test     | Enabled (`VK_COMPARE_OP_LESS`)                                             |
| Face culling   | Back-face culling                                                          |
| Shading        | Blinn-Phong with up to 8 dynamic lights                                    |
| Vertex format  | `Vertex` -- position, normal, color, UV                                    |
| Descriptors    | Set 0: view/proj UBO. Set 1: light UBO. Set 2: base color texture sampler. |
| Push constants | `mat4 model` -- per-entity model matrix                                    |

The 3D pipeline renders all opaque scene geometry first.

### Debug Wireframe Pipeline

| Setting        | Value                                                               |
| -------------- | ------------------------------------------------------------------- |
| Depth test     | Enabled (`VK_COMPARE_OP_LESS_OR_EQUAL`)                             |
| Depth write    | Disabled (wireframes don't occlude other objects)                   |
| Face culling   | None                                                                |
| Polygon mode   | `VK_POLYGON_MODE_LINE` (requires `fillModeNonSolid` device feature) |
| Vertex format  | Same `Vertex` as 3D pipeline                                        |
| Descriptors    | Same layout as 3D pipeline (set 0: UBOs, set 1: material texture)   |
| Push constants | Same `mat4 model` as 3D pipeline                                    |

The debug wireframe pipeline renders **after** 3D geometry and **before** the UI overlay, but only when the debug overlay is enabled (`debugOverlayEnabled_ == true`). Debug entities are stored in a separate `debugEntities_` list and use meshes from the shared combined vertex/index buffers. Created alongside the 3D pipeline in `createGraphicsPipeline()` by modifying rasterizer and depth-stencil state.

### UI Overlay Pipeline

| Setting        | Value                                                                 |
| -------------- | --------------------------------------------------------------------- |
| Depth test     | Disabled                                                              |
| Face culling   | None                                                                  |
| Blending       | `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` alpha blending                      |
| Vertex format  | `UIVertex` -- position, UV, color                                     |
| Descriptors    | `COMBINED_IMAGE_SAMPLER` bound to the font atlas (512x512 `R8_UNORM`) |
| Push constants | `vec2 screenSize` -- for pixel-to-NDC conversion                      |

The UI pipeline renders **after** all 3D geometry but **before** `vkCmdEndRenderPass`. It uses host-visible, persistently-mapped vertex buffers (one per frame-in-flight, max 4096 vertices) so that C# can write UI vertices directly without staging copies. The font atlas is baked once at init from `assets/fonts/RobotoMono-Regular.ttf` using stb_truetype.

## Where to Edit

:::tip Where to Edit -- Adding a New Native Function
Every new function exposed to C# requires changes in **three files**:

1. **`native/renderer.h` + `native/renderer.cpp`** -- Add the method to the `VulkanRenderer` class and implement it.
2. **`native/bridge.cpp`** -- Add an `extern "C"` wrapper that calls the new method on `g_renderer`, wrapped in a try/catch.
3. **`managed/NativeBridge.cs`** -- Add a `[DllImport("renderer")]` static extern declaration matching the C function signature.
   :::

:::tip Where to Edit -- Adding a New Shader

1. Create the `.vert` and/or `.frag` file in `native/shaders/`.
2. In the `Makefile`, add a new SPV variable (e.g., `MY_VERT_SPV = $(SHADER_DIR)/my_vert.spv`) and a corresponding compilation rule (`glslc $< -o $@`).
3. Add the new SPV target to the `shaders:` dependency list so it gets built automatically.
4. In `renderer.cpp`, load the new SPIR-V binary and create the pipeline that uses it.
   :::

:::tip Where to Edit -- Adding a New C# File
The project uses two folders for C# code:

- **Engine files** (`managed/`) -- Add the path to `MANAGED_CS` in the Makefile. These are stable engine internals (World, Components, NativeBridge, etc.).
- **Game files** (`game_logic/`) -- Add the path to `GAMELOGIC_CS_FILES` in the Makefile. These are user-editable game logic files and are hot-reloadable with `make dev`.

The `.csproj` uses `EnableDefaultCompileItems` so it discovers new files automatically — no project file changes needed. Only the Makefile needs updating.
:::

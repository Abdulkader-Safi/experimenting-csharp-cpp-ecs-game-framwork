# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make run             # Build everything and run the Vulkan viewer
make dev             # Build and run with hot reload (edit C# game logic live)
make viewer          # Build shaders + C++ dylib + physics dylib + C# exe (no run)
make app             # Build macOS .app bundle
make shaders         # Compile GLSL → SPIR-V only
make clean           # Remove all build artifacts
```

The build pipeline has four stages: GLSL→SPIR-V (`glslc`), C++ renderer→`build/librenderer.dylib` (CMake), joltc→`build/libjoltc.dylib` (CMake), C#→`build/Viewer.exe` (`mcs`). `make run` sets `DYLD_LIBRARY_PATH` and `VK_ICD_FILENAMES` automatically. There is no test suite — verification is done visually via `make run`.

## Architecture

C# (Mono) owns the main loop and all game logic. C++ owns Vulkan rendering. Jolt Physics runs via a separate C API (`joltc`). All cross-language communication is via P/Invoke.

```
C# Engine (managed/)              C++ Renderer (native/)        Physics (native/joltc/)
  World, Components, Systems         VulkanRenderer class          Jolt Physics C API
  NativeBridge.cs ──DllImport──>     bridge.cpp (extern "C")      libjoltc.dylib
  PhysicsBridge.cs ──DllImport──>                                  (git submodule)
```

Two-folder structure separates engine from game code:

- **`managed/`** — Engine (user doesn't edit): Viewer.cs, World.cs, Components.cs, NativeBridge.cs, PhysicsBridge.cs, PhysicsWorld.cs, FreeCameraState.cs, HotReload.cs
- **`game_logic/`** — Game code (user edits): Game.cs, Systems.cs, GameConstants.cs

**Adding a new renderer function** requires changes in three places:

1. `native/renderer.h` + `renderer.cpp` — implementation
2. `native/bridge.cpp` — `extern "C"` wrapper
3. `managed/NativeBridge.cs` — `[DllImport("renderer")]` declaration

**Adding a new C# game file:** add to `game_logic/` and `GAMELOGIC_CS_FILES` in the Makefile. Auto-included in hot reload.

**Adding a new engine C# file:** add to `managed/` and both `MANAGED_CS` and `ENGINE_CS` in the Makefile.

**Adding a new GLSL shader:** add compilation rules and SPV targets in the Makefile, then add to the `shaders:` dependency list.

## ECS Pattern

- **Components** are plain C# classes (data only, no behavior, must be reference types) in `managed/Components.cs`
- **Systems** are `static void Name(World world)` methods that query and mutate, in `game_logic/Systems.cs`
- **System order matters:** register input systems first, `RenderSyncSystem` always last
- `world.Query(typeof(A), typeof(B))` returns entities with ALL specified components
- Despawning cascade-deletes children (via `Hierarchy` component) and cleans up renderer + physics resources

Built-in system chain (registered in `game_logic/Game.cs`):

```
InputMovement → Timer → Physics → FreeCamera → CameraFollow → LightSync → HierarchyTransform → DebugOverlay → DebugColliderRender → RenderSync
```

## Physics (Jolt via joltc)

Physics uses [joltc](https://github.com/amerkoleci/joltc) (C API for Jolt Physics) as a git submodule at `native/joltc/`. The integration follows the same P/Invoke pattern as the renderer.

- **`PhysicsBridge.cs`** — `[DllImport("joltc")]` declarations, blittable structs (`JPH_Vec3`, `JPH_RVec3`, `JPH_Quat`), enums (`JPH_MotionType`)
- **`PhysicsWorld.cs`** — Engine-layer singleton managing Jolt lifecycle, body tracking (`Dictionary<int, uint>` entityId→bodyId), fixed timestep (1/60s, max 4 steps/frame)
- **`PhysicsSystem`** (in `game_logic/Systems.cs`) — Three-phase: create bodies → step simulation → sync transforms back to ECS
- **`Rigidbody`** + **`Collider`** components (in `managed/Components.cs`) — Separate dynamics properties from shape definition

Key details:

- `JPH_RVec3` uses `double` (positions), `JPH_Vec3` uses `float` (velocities/forces)
- No single `RemoveAndDestroyBody` — must call `RemoveBody` then `DestroyBody` separately
- Do NOT call `JPH_Shape_Destroy` after body creation — Jolt ref-counts shapes internally
- `PhysicsWorld` singleton survives hot reloads (engine layer); `PhysicsSystem` is hot-reloadable (game layer)
- `world.Reset()` calls `RemoveAllBodies()` before despawning; `world.Despawn()` calls `RemoveBody()`

## Vulkan Pipelines

The renderer runs three graphics pipelines within the same render pass:

1. **3D Scene Pipeline** — depth testing, back-face culling, Blinn-Phong shading. `Vertex` (pos/normal/color/uv), UBOs for view/proj + lights, push constants for per-entity model matrix. Shaders: `shader.vert`, `shader.frag`.

2. **Debug Wireframe Pipeline** — `VK_POLYGON_MODE_LINE`, no culling, no depth write, `LESS_OR_EQUAL` depth test. Reuses 3D pipeline shaders, layout, and vertex/index buffers. Renders debug entities (collider shapes) only when debug overlay is enabled. Requires `fillModeNonSolid` device feature.

3. **UI Overlay Pipeline** — no depth test, alpha blending, no culling. `UIVertex` (pos/uv/color), font atlas sampler, push constant `vec2 screenSize`. Shaders: `ui.vert`, `ui.frag`. Renders after 3D scene and debug wireframes.

## Hot Reload (Dev Mode)

`make dev` splits C# into three assemblies:

- **Engine.dll** (stable) — World, Components, NativeBridge, PhysicsBridge, PhysicsWorld, FreeCameraState
- **GameLogic.dll** (hot-reloadable) — all `game_logic/*.cs` files
- **ViewerDev.exe** — Viewer + HotReload, compiled with `-define:HOT_RELOAD`

On save, `FileSystemWatcher` recompiles `GameLogic.dll`, calls `world.Reset()` (which removes all physics bodies, despawns entities, clears lights), then re-invokes `Game.Setup(world)` from the new assembly.

Limitations: only `game_logic/` is hot-reloadable; old assemblies leak (Mono can't unload); static fields reset on reload.

## Mono/mcs C# Compatibility

The project compiles with `mcs` (Mono), not `dotnet build`. Key limitations:

- **No C# 7 ValueTuple named fields.** Named field access (`.name`) fails with "inaccessible due to protection level". Use plain classes instead.
- **`ECS.Timer` shadows `System.Threading.Timer`.** Always fully qualify as `System.Threading.Timer` inside the `ECS` namespace.

## Documentation Sites

Two Rspress 2.x sites:

```bash
cd docs && bun install && bun run dev              # User-facing docs
cd technical_docs && bun install && bun run dev    # Renderer internals docs
```

## Key Constraints

- MoltenVK ICD path: `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` (not under `/share/`)
- MoltenVK requires `VK_KHR_portability_enumeration` + `VK_KHR_portability_subset` extensions
- `cgltf.h`, `stb_truetype.h`, `stb_image.h` each need `#define *_IMPLEMENTATION` in exactly one `.cpp` file
- Fragment shader max 8 dynamic lights (`MAX_LIGHTS` in shader and C++)
- Font atlas requires `assets/fonts/RobotoMono-Regular.ttf` — falls back to 1x1 white placeholder if missing
- GLFW/Vulkan tests fail in headless/subprocess contexts (no window server) — expected on macOS
- First joltc build fetches JoltPhysics via CMake FetchContent (~1 min); subsequent builds are fast

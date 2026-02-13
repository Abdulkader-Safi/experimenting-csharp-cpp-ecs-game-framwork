# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make viewer          # Build shaders + C++ dylib + C# exe
make run             # Build and run the Vulkan viewer
make dev             # Build and run with hot reload (edit C# game logic live)
make app             # Build macOS .app bundle
make shaders         # Compile GLSL → SPIR-V only
make clean           # Remove all build artifacts
make all             # Build hello demo (basic P/Invoke test)
```

The viewer build does three things in sequence:

1. `glslc` compiles 4 shaders (`.vert`/`.frag`) → `.spv` in `build/shaders/`
2. CMake builds `native/` → `build/librenderer.dylib`
3. `mcs` compiles all `managed/*.cs` + `game_logic/*.cs` → `build/Viewer.exe`

`make run` sets `DYLD_LIBRARY_PATH` and `VK_ICD_FILENAMES` (MoltenVK) automatically. There is no test suite — verification is done by running the viewer (`make run`).

## Documentation Sites

Two Rspress doc sites live in the repo:

```bash
# User-facing docs
cd docs && bun install && bun run dev        # Dev server
cd docs && bun run build                     # Production build

# Technical / renderer internals docs
cd technical_docs && bun install && bun run dev
cd technical_docs && bun run build
```

Both use Rspress 2.x (`@rspress/core`). The `docs/` site covers ECS usage and features. The `technical_docs/` site covers Vulkan renderer internals (pipeline setup, shaders, data structures, bridge API).

## Architecture

Two-folder structure separates engine from game code:

```
managed/                        ← ENGINE (user doesn't edit)
  Viewer.cs                       Main loop, renderer init, #if HOT_RELOAD guards
  World.cs                        ECS core (spawn, query, systems)
  Components.cs                   Built-in component types
  NativeBridge.cs                 P/Invoke bindings
  FreeCameraState.cs              Debug camera state
  HotReload.cs                    File watcher + recompiler
  SaFiEngine.csproj               IntelliSense (includes game_logic/)

game_logic/                     ← GAME CODE (user edits these)
  Game.cs                         Scene setup + system registration
  Systems.cs                      Per-frame system logic
  GameConstants.cs                Tunable constants
```

C# (Mono) drives the main loop and ECS game logic. C++ handles Vulkan rendering. They communicate via P/Invoke through a shared library.

```
C# World (managed/)             C++ VulkanRenderer (native/)
  Entities + Components   ──P/Invoke──>  GPU resources
  Systems (per-frame)     ──────────>    Draw calls
  NativeBridge.cs         ──DllImport──> bridge.cpp (extern "C")
```

**Adding a new native function requires changes in three places:**

1. `native/bridge.cpp` — `extern "C"` wrapper calling into `VulkanRenderer`
2. `native/renderer.h` + `renderer.cpp` — implementation on the C++ class
3. `managed/NativeBridge.cs` — `[DllImport("renderer")]` declaration

**Adding a new C# game file:** add it to `game_logic/`. It will be auto-included in hot reload and picked up by the Makefile's `GAMELOGIC_CS_FILES` list (add to Makefile for `make viewer`/`make run`).

**Adding a new engine C# file:** add it to the `MANAGED_CS` list in the `Makefile`.

**Adding a new GLSL shader:** add compilation rules and SPV targets in the `Makefile`, then add to the `shaders:` dependency list.

## Dual Vulkan Pipeline

The renderer runs two graphics pipelines within the same render pass:

1. **3D Scene Pipeline** — depth testing, back-face culling, Blinn-Phong shading. Uses `Vertex` (pos/normal/color), UBOs for view/proj + lights, push constants for per-entity model matrix. Shaders: `shader.vert`, `shader.frag`.

2. **UI Overlay Pipeline** — no depth test, alpha blending (`SRC_ALPHA/ONE_MINUS_SRC_ALPHA`), no culling. Uses `UIVertex` (pos/uv/color), a `COMBINED_IMAGE_SAMPLER` for the font atlas, push constant `vec2 screenSize`. Shaders: `ui.vert`, `ui.frag`. Renders after the 3D scene, before `vkCmdEndRenderPass`.

The UI pipeline uses host-visible, persistently-mapped vertex buffers (one per frame-in-flight, max 4096 vertices). The font atlas is a 512x512 `R8_UNORM` texture baked from TTF via stb_truetype at init.

## ECS Pattern

- **Components** are plain C# classes (data only, no behavior, must be reference types)
- **Systems** are `static void SystemName(World world)` methods that query and mutate
- **System order matters:** register input systems first, `RenderSyncSystem` always last
- `world.DeltaTime` is set each frame from `NativeBridge.renderer_get_delta_time()`
- Despawning an entity cascade-deletes all children (via `Hierarchy` component)
- `world.Query(typeof(A), typeof(B))` returns entities with ALL specified components

Built-in system chain (registered in `game_logic/Game.cs`):

```
InputMovement → Timer → FreeCamera → CameraFollow → LightSync → HierarchyTransform → DebugOverlay → RenderSync
```

## Hot Reload (Dev Mode)

`make dev` splits C# into three assemblies for live code reloading:

- **Engine.dll** (stable) — World, Components, NativeBridge, FreeCameraState
- **GameLogic.dll** (hot-reloadable) — all `game_logic/*.cs` files (Game.cs, Systems.cs, GameConstants.cs)
- **ViewerDev.exe** — Viewer + HotReload, compiled with `-define:HOT_RELOAD`

A single `FileSystemWatcher` monitors `game_logic/` for `.cs` changes. On save, it auto-discovers all `.cs` files in the directory, recompiles `GameLogic.dll`, loads the new assembly via reflection, and swaps system delegates by name. Game state survives because it lives in Engine.dll.

`make run` and `make app` are completely unaffected — they compile everything into a single `Viewer.exe` with no hot reload code (`#if HOT_RELOAD` guards).

**Limitations:**

- Only `game_logic/` files are hot-reloadable. Engine changes (`managed/`) require restart.
- New system methods compile but won't execute until registered in Game.cs (restart needed for new systems).
- Old assemblies leak memory (Mono can't unload). Restart after many reloads.
- Static fields in Systems.cs (e.g., `wasF3Pressed_`) and GameConstants values reset on reload.

## Mono/mcs C# Compatibility

The project compiles with `mcs` (Mono), not `dotnet build`. Key limitations:

- **No C# 7 ValueTuple named fields.** `(string name, Action action)` tuples compile but named field access (`.name`) fails with "inaccessible due to protection level". Use plain classes instead.
- **`ECS.Timer` shadows `System.Threading.Timer`.** Inside the `ECS` namespace, always fully qualify as `System.Threading.Timer`.

## Key Constraints

- MoltenVK ICD path: `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` (not under `/share/`)
- MoltenVK requires `VK_KHR_portability_enumeration` instance extension + `VK_KHR_portability_subset` device extension
- `cgltf.h` needs `#define CGLTF_IMPLEMENTATION` in exactly one `.cpp` file (same for `stb_truetype.h` with `STB_TRUETYPE_IMPLEMENTATION`)
- `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` ≠ `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT` — different enum types
- GLFW/Vulkan tests fail in headless/subprocess contexts (no window server) — expected on macOS
- Fragment shader supports max 8 dynamic lights (defined as `MAX_LIGHTS` in shader and C++)
- Push constants carry per-entity model matrix (3D) or screen size (UI) — separate pipeline layouts
- CMake generates `compile_commands.json` symlinked to repo root for IDE C++ intellisense
- Font atlas requires `assets/fonts/RobotoMono-Regular.ttf` — falls back to 1x1 white placeholder if missing

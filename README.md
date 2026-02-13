# Safi ECS Game Engine

A Vulkan-based ECS game engine built with C# and C++. C# drives the main loop and all game logic through an Entity Component System, while a C++ shared library handles Vulkan rendering — connected via Mono P/Invoke.

![Safi ECS Game Engine](assets/img5.png)

## Features

- **Entity Component System** — Bevy-inspired ECS with entity spawning/despawning, component queries, and ordered system execution
- **Vulkan Renderer** — C++ rendering backend with swapchain management, depth testing, and automatic window resize handling
- **glTF Model Loading** — Load `.glb`/`.gltf` models via cgltf with vertex positions, normals, and colors
- **Multi-Entity Rendering** — Spawn multiple entities with independent transforms using per-entity push constants
- **Procedural Primitives** — Generate box, sphere, plane, cylinder, and capsule meshes without external files
- **Dynamic Lighting** — Up to 8 simultaneous lights (directional, point, spot) with Blinn-Phong shading
- **Camera System** — Orbit (third-person), first-person, and free debug fly camera with mouse look, zoom, and mode switching
- **Keyboard & Mouse Input** — WASD/arrow key movement, mouse look, scroll wheel zoom, mouse buttons, cursor lock toggle
- **Parent-Child Hierarchy** — Entity relationships with automatic world-space transform propagation
- **Debug Overlay** — FPS counter, delta time, and entity count rendered as GPU text via a second Vulkan pipeline with stb_truetype font atlas; toggled with F3. When enabled, wireframe outlines are drawn around all physics colliders using a dedicated wireframe pipeline (`VK_POLYGON_MODE_LINE`) with per-collider configurable colors via `DebugColor`
- **Timers** — Countdown and interval timers for cooldowns, spawning, and delays
- **Delta Time** — Frame-independent movement via native-side GLFW timing
- **Runtime Spawn/Despawn** — Create and destroy entities at runtime with automatic native resource cleanup
- **Physics** — Jolt Physics integration via [joltc](https://github.com/amerkoleci/joltc) C API. Rigid body dynamics (static/kinematic/dynamic), collider shapes (box, sphere, capsule, cylinder, plane), gravity, friction, restitution, and damping. Fixed timestep (1/60s) with accumulator pattern.
- **Hot Reload** — `make dev` watches `game_logic/` and live-reloads on save without restarting. Editing any file re-runs `Game.Setup`, so scene changes (new entities, lights, system registration) take effect immediately
- **macOS App Bundle** — Packageable as a standalone `.app` with embedded runtime and assets

## Quick Start

### Prerequisites

- [Mono](https://www.mono-project.com/) (`mcs`, `mono`)
- [CMake](https://cmake.org/) (>= 3.20)
- [Vulkan SDK](https://vulkan.lunarg.com/) (MoltenVK on macOS)
- [GLFW](https://www.glfw.org/) and [GLM](https://github.com/g-truc/glm) — `brew install glfw glm`
- **glslc** (SPIR-V shader compiler, included with Vulkan SDK)

### Installation

```sh
git clone --recursive https://github.com/Abdulkader-Safi/Safi-ECS-Game-Engine.git
cd Safi-ECS-Game-Engine
```

If you already cloned without `--recursive`, fetch the submodules separately:

```sh
git submodule update --init --recursive
```

### Build & Run

```sh
make run
```

This compiles GLSL shaders to SPIR-V, builds the C++ shared library via CMake, compiles all C# sources with Mono, and launches the viewer with the correct MoltenVK environment.

### Controls

#### Player Camera (default)

| Input               | Action                             |
| ------------------- | ---------------------------------- |
| WASD / Arrow Keys   | Rotate player entity               |
| ESC                 | Toggle cursor lock for mouse look  |
| Mouse (when locked) | Orbit camera yaw/pitch             |
| Q / E               | Camera yaw                         |
| R / F               | Camera pitch                       |
| Scroll wheel        | Zoom in/out (third-person)         |
| TAB                 | Toggle first-person / third-person |

#### Free Camera (debug)

| Input               | Action                                          |
| ------------------- | ----------------------------------------------- |
| 0                   | Activate free camera (debug mode only)          |
| 1                   | Deactivate free camera, return to player camera |
| WASD                | Fly forward/back/left/right                     |
| Q / E               | Fly down / up                                   |
| Mouse (when locked) | Look around                                     |
| ESC                 | Toggle cursor lock                              |

#### Global

| Input | Action                                                              |
| ----- | ------------------------------------------------------------------- |
| F3    | Toggle debug overlay (FPS, DT, entity count, collider wireframes)   |

## Architecture

```
C# Engine (managed/)               C++ VulkanRenderer (native/)
  World, Components, NativeBridge     Vulkan instance, swapchain, pipeline
C# Game (game_logic/)                 Vertex/index buffers, push constants
  Game.cs, Systems.cs                 UBOs for view/proj + lighting
       └──── P/Invoke (DllImport) ────→ bridge.cpp (extern "C")
```

The C# side owns the game loop and all ECS logic. Each frame it queries entities, runs systems in order, and pushes transforms and light data to the native renderer. The C++ side manages all GPU resources, shader pipelines, and draw calls.

### ECS Pattern

- **Components** — Plain C# classes (data only): `Color`, `Transform`, `MeshComponent`, `Movable`, `Light`, `Camera`, `Rigidbody`, `Collider`
- **Systems** — Static methods that query and mutate the world: `InputMovementSystem` → `TimerSystem` → `PhysicsSystem` → `FreeCameraSystem` → `CameraFollowSystem` → `LightSyncSystem` → `HierarchyTransformSystem` → `DebugOverlaySystem` → `DebugColliderRenderSystem` → `RenderSyncSystem`
- **World** — Manages entity lifecycles, component storage, system registration, and delta time

System execution order matters — `RenderSyncSystem` must always run last. Systems are registered in `game_logic/Game.cs`.

### Adding a New Native Function

Changes are required in three places:

1. `native/bridge.cpp` — `extern "C"` wrapper
2. `native/renderer.h` + `renderer.cpp` — Implementation on the `VulkanRenderer` class
3. `managed/NativeBridge.cs` — `[DllImport("renderer")]` declaration

New engine C# files go in `managed/` (add to `MANAGED_CS` in the Makefile). New game C# files go in `game_logic/` (add to `GAMELOGIC_CS_FILES` in the Makefile).

### IDE Setup

The repo includes `SaFiEngine.sln` and `managed/SaFiEngine.csproj` for C# IntelliSense (autocomplete, go-to-definition). These are used by the IDE only — the actual build uses `mcs` via the Makefile. Run `dotnet restore` once after cloning to generate the project assets the language server needs.

## Make Targets

| Target         | Description                            |
| -------------- | -------------------------------------- |
| `make run`     | Build everything and run the viewer    |
| `make dev`     | Build and run with hot reload          |
| `make viewer`  | Build shaders + native lib + C# exe    |
| `make app`     | Build macOS `.app` bundle              |
| `make shaders` | Compile GLSL → SPIR-V only             |
| `make clean`   | Remove all build artifacts             |
| `make all`     | Build hello demo (basic P/Invoke test) |
| `make help`    | Show available targets                 |

## Project Structure

```
.
├── Makefile                          # Build automation
├── SaFiEngine.sln                   # Solution file (IDE project discovery)
├── build-app.sh                     # macOS .app packaging script
├── native/
│   ├── joltc/                       # Jolt Physics C API (git submodule)
│   ├── CMakeLists.txt               # CMake config for shared library
│   ├── renderer.cpp                 # Vulkan renderer implementation
│   ├── renderer.h                   # Renderer class declaration
│   ├── bridge.cpp                   # C-linkage P/Invoke bridge
│   ├── shaders/
│   │   ├── shader.vert              # Vertex shader (GLSL 4.5)
│   │   ├── shader.frag              # Fragment shader (Blinn-Phong lighting)
│   │   ├── ui.vert                  # UI vertex shader (2D pixel-to-NDC)
│   │   └── ui.frag                  # UI fragment shader (font atlas sampling)
│   └── vendor/
│       ├── cgltf.h                  # glTF 2.0 parsing library
│       ├── stb_truetype.h           # Font rasterization library
│       └── stb_image.h             # Image decoding library
├── managed/                         # ENGINE (stable, user doesn't edit)
│   ├── SaFiEngine.csproj           # Project file (IDE IntelliSense only, not used by build)
│   ├── Viewer.cs                    # Entry point — calls Game.Setup(), runs game loop
│   ├── World.cs                     # ECS world: entities, components, systems
│   ├── Components.cs                # Color, Transform, MeshComponent, Movable, Light, Camera, Rigidbody, Collider
│   ├── NativeBridge.cs              # P/Invoke bindings to C++ renderer
│   ├── PhysicsBridge.cs             # P/Invoke bindings to joltc physics library
│   ├── PhysicsWorld.cs              # Jolt Physics lifecycle, body tracking, fixed timestep
│   ├── FreeCameraState.cs           # Static state for the debug free camera
│   └── HotReload.cs                 # File watcher + recompiler (dev mode)
├── game_logic/                      # GAME CODE (user edits these)
│   ├── Game.cs                      # Scene setup + system registration
│   ├── Systems.cs                   # Per-frame system logic
│   └── GameConstants.cs             # Tunable config values (debug, sensitivity, speed)
├── assets/
│   └── fonts/
│       └── RobotoMono-Regular.ttf   # Monospace font for debug overlay
├── models/                          # glTF models (.glb)
├── docs/                            # Rspress user-facing documentation site
├── technical_docs/                  # Rspress technical/renderer internals docs
├── plans/                           # Roadmap and planning docs
└── build/                           # Generated build artifacts
```

## Documentation

The project includes two [Rspress](https://rspress.dev/) documentation sites:

- **`docs/`** — User-facing docs covering architecture, ECS usage, and the feature roadmap
- **`technical_docs/`** — Renderer internals: Vulkan pipeline setup, shaders, data structures, bridge API

```sh
cd docs && bun install && bun run dev              # User docs dev server
cd technical_docs && bun install && bun run dev    # Technical docs dev server
```

## Roadmap

The engine currently implements 23 core features. The [full roadmap](plans/features.md) tracks 100+ planned features across rendering (textures, PBR, shadows), animation (skeletal, blending), physics (collision, rigidbody), scene management, audio, UI, AI, and cross-platform support including web export.

## Support

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J11RP7T5)

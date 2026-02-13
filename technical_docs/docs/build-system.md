# Build & Compilation

## Makefile Targets

| Target         | Description                                                    |
| -------------- | -------------------------------------------------------------- |
| `make viewer`  | Build shaders + C++ dylib + C# exe                             |
| `make run`     | Build everything and run the viewer                            |
| `make dev`     | Build and run with hot reload (edit game logic live)           |
| `make app`     | Build macOS .app bundle                                        |
| `make shaders` | Compile GLSL shaders to SPIR-V only                            |
| `make all`     | Build hello demo (basic P/Invoke test)                         |
| `make clean`   | Remove all build artifacts (`build/`, `compile_commands.json`) |

`make run` is the primary development command. `make dev` enables hot reload for game logic.

## Shader Compilation

GLSL shaders are compiled to SPIR-V using `glslc` (Vulkan SDK tool):

```makefile
$(VERT_SPV): native/shaders/shader.vert | $(SHADER_DIR)
    glslc $< -o $@

$(FRAG_SPV): native/shaders/shader.frag | $(SHADER_DIR)
    glslc $< -o $@

$(UI_VERT_SPV): native/shaders/ui.vert | $(SHADER_DIR)
    glslc $< -o $@

$(UI_FRAG_SPV): native/shaders/ui.frag | $(SHADER_DIR)
    glslc $< -o $@
```

Input files in `native/shaders/`, output in `build/shaders/`:
| Source | Output |
|--------|--------|
| `shader.vert` | `build/shaders/vert.spv` |
| `shader.frag` | `build/shaders/frag.spv` |
| `ui.vert` | `build/shaders/ui_vert.spv` |
| `ui.frag` | `build/shaders/ui_frag.spv` |

## CMake Configuration

`native/CMakeLists.txt` builds the shared library:

```cmake
cmake_minimum_required(VERSION 3.20)
project(renderer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_library(renderer SHARED
    renderer.cpp
    bridge.cpp
)

target_include_directories(renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

target_link_libraries(renderer PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    "-framework Cocoa"
    "-framework IOKit"
)
```

Output: `build/librenderer.dylib`

The Makefile runs CMake with `CMAKE_EXPORT_COMPILE_COMMANDS=ON` and symlinks `compile_commands.json` to the repo root for IDE intellisense.

**Dependencies** (installed via Homebrew):

- Vulkan SDK (includes `glslc`, Vulkan headers, MoltenVK)
- GLFW 3
- GLM

**Vendor headers** (in `native/vendor/`, no separate build):

- `cgltf.h` — glTF 2.0 parser (single-header, needs `#define CGLTF_IMPLEMENTATION` in one .cpp)
- `stb_truetype.h` — TrueType font rasterizer (needs `#define STB_TRUETYPE_IMPLEMENTATION`)
- `stb_image.h` — Image decoder (needs `#define STB_IMAGE_IMPLEMENTATION`)

All three `#define` directives are at the top of `renderer.cpp`.

## C# Compilation

The Makefile uses two separate file lists — engine code and game logic — combined into `VIEWER_CS`:

```makefile
MANAGED_CS = managed/Viewer.cs managed/World.cs managed/Components.cs \
             managed/NativeBridge.cs managed/FreeCameraState.cs \
             managed/PhysicsBridge.cs managed/PhysicsWorld.cs
GAMELOGIC_CS_FILES = game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
VIEWER_CS = $(MANAGED_CS) $(GAMELOGIC_CS_FILES)

$(VIEWER_EXE): $(VIEWER_CS)
    mcs -out:$@ $(VIEWER_CS)
```

Uses the Mono C# compiler (`mcs`). All source files are listed explicitly in `MANAGED_CS` (engine) and `GAMELOGIC_CS_FILES` (game logic) — there is no automatic file discovery.

:::info IDE IntelliSense
The repo includes `SaFiEngine.sln` (repo root) and `managed/SaFiEngine.csproj` for C# language server support (autocomplete, go-to-definition, error checking). These files are **not used by the build** — the Makefile drives compilation via `mcs`. After cloning, run `dotnet restore` once to generate `managed/obj/project.assets.json`, which the language server needs to resolve `System.*` types.
:::

## Dev Mode (Hot Reload)

`make dev` splits C# into three assemblies for live code reloading:

```makefile
# Engine (stable) — World, Components, NativeBridge, FreeCameraState, PhysicsBridge, PhysicsWorld
ENGINE_CS = managed/World.cs managed/Components.cs \
            managed/NativeBridge.cs managed/FreeCameraState.cs \
            managed/PhysicsBridge.cs managed/PhysicsWorld.cs
ENGINE_DLL = $(BUILD_DIR)/Engine.dll

# Game logic (hot-reloadable) — Game.cs, Systems.cs, GameConstants.cs
GAMELOGIC_CS = game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
GAMELOGIC_DLL = $(BUILD_DIR)/GameLogic.dll

# Viewer with hot reload support
VIEWERDEV_CS = managed/Viewer.cs managed/HotReload.cs
VIEWERDEV_EXE = $(BUILD_DIR)/ViewerDev.exe
```

| Assembly        | Contents                                                      | Reloadable? |
| --------------- | ------------------------------------------------------------- | ----------- |
| `Engine.dll`    | World, Components, NativeBridge, FreeCameraState, PhysicsBridge, PhysicsWorld | No          |
| `GameLogic.dll` | Game.cs, Systems.cs, GameConstants.cs                         | Yes         |
| `ViewerDev.exe` | Viewer.cs + HotReload.cs (compiled with `-define:HOT_RELOAD`) | No          |

A `FileSystemWatcher` in `HotReload.cs` monitors `game_logic/` for `.cs` changes. On save, it auto-discovers all `.cs` files in the directory, recompiles `GameLogic.dll`, and loads the new assembly via reflection. The reload then looks for a `Game.Setup(World)` method in the new assembly:

- **If found** — calls `world.Reset()` (despawns all entities, clears component stores, clears systems, clears all 8 light slots on the native side, resets entity IDs) then re-invokes `Game.Setup(world)` from the new assembly. This makes scene changes (new entities, lights, system registration order) take effect immediately.
- **If not found** — falls back to swapping system delegates by name (systems-only reload).

`make run` and `make app` are unaffected — they compile everything into a single `Viewer.exe` with no hot reload code (`#if HOT_RELOAD` guards).

## Runtime Environment

`make run` sets environment variables before executing:

```bash
DYLD_LIBRARY_PATH=build:/opt/homebrew/lib \
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
    mono build/Viewer.exe
```

- `DYLD_LIBRARY_PATH` — locates `librenderer.dylib` and `libjoltc.dylib` (in `build/`) and MoltenVK/GLFW (in `/opt/homebrew/lib`)
- `VK_ICD_FILENAMES` — tells the Vulkan loader where to find the MoltenVK ICD. Note: the path is under `/etc/` not `/share/` (Homebrew-specific)
- `mono` — runs the compiled .NET executable

:::tip Where to Edit
**Adding a new shader**: Create the `.vert`/`.frag` file in `native/shaders/`. In the `Makefile`, add a new SPV variable (e.g., `NEW_VERT_SPV`), add a compilation rule, and add it to the `shaders:` dependency list.

**Adding a new C++ source file**: Add it to `add_library(renderer SHARED ...)` in `native/CMakeLists.txt`. CMake will pick it up on next build.

**Adding a new C# file**: Engine files go in `managed/` — add the path to `MANAGED_CS` in the Makefile. Game files go in `game_logic/` — add the path to `GAMELOGIC_CS_FILES`. Game files in `game_logic/` are automatically hot-reloadable with `make dev`. The `.csproj` discovers files automatically, so no project file update is needed.
:::

# Build & Compilation

## Makefile Targets

| Target         | Description                                                    |
| -------------- | -------------------------------------------------------------- |
| `make viewer`  | Build shaders + C++ dylib + C# exe                             |
| `make run`     | Build everything and run the viewer                            |
| `make app`     | Build macOS .app bundle                                        |
| `make shaders` | Compile GLSL shaders to SPIR-V only                            |
| `make all`     | Build hello demo (basic P/Invoke test)                         |
| `make clean`   | Remove all build artifacts (`build/`, `compile_commands.json`) |

`make run` is the primary development command.

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

```makefile
VIEWER_CS = managed/Viewer.cs managed/ecs/World.cs managed/ecs/Components.cs \
            managed/ecs/Systems.cs managed/ecs/NativeBridge.cs \
            managed/ecs/GameConstants.cs managed/ecs/FreeCameraState.cs

$(VIEWER_EXE): $(VIEWER_CS)
    mcs -out:$@ $(VIEWER_CS)
```

Uses the Mono C# compiler (`mcs`). All source files are listed explicitly in `VIEWER_CS` — there is no automatic file discovery.

:::info IDE IntelliSense
The repo includes `SaFiEngine.sln` (repo root) and `managed/SaFiEngine.csproj` for C# language server support (autocomplete, go-to-definition, error checking). These files are **not used by the build** — the Makefile drives compilation via `mcs`. After cloning, run `dotnet restore` once to generate `managed/obj/project.assets.json`, which the language server needs to resolve `System.*` types.
:::

## Runtime Environment

`make run` sets environment variables before executing:

```bash
DYLD_LIBRARY_PATH=build:/opt/homebrew/lib \
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
    mono build/Viewer.exe
```

- `DYLD_LIBRARY_PATH` — locates `librenderer.dylib` (in `build/`) and MoltenVK/GLFW (in `/opt/homebrew/lib`)
- `VK_ICD_FILENAMES` — tells the Vulkan loader where to find the MoltenVK ICD. Note: the path is under `/etc/` not `/share/` (Homebrew-specific)
- `mono` — runs the compiled .NET executable

:::tip Where to Edit
**Adding a new shader**: Create the `.vert`/`.frag` file in `native/shaders/`. In the `Makefile`, add a new SPV variable (e.g., `NEW_VERT_SPV`), add a compilation rule, and add it to the `shaders:` dependency list.

**Adding a new C++ source file**: Add it to `add_library(renderer SHARED ...)` in `native/CMakeLists.txt`. CMake will pick it up on next build.

**Adding a new C# file**: Add the file path to the `VIEWER_CS` variable in the `Makefile`. The `.csproj` discovers files automatically, so no project file update is needed.
:::

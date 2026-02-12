# Build & Run

## Quick Build

```bash
make clean && make run
```

This compiles shaders, builds the C++ shared library, compiles all C# files, and launches the viewer.

## Build Targets

| Command                           | Description                          |
| --------------------------------- | ------------------------------------ |
| `make all` / `make run`           | Build and run the hello demo         |
| `make viewer` / `make run-viewer` | Build and run the Vulkan glTF viewer |
| `make app`                        | Build a macOS `.app` bundle          |
| `make clean`                      | Remove all build artifacts           |

## macOS App Bundle

To build and launch a standalone `.app`:

```bash
make app
open build/Viewer.app
```

## What the Build Does

1. **Compiles shaders** — `glslc` compiles `.vert` and `.frag` GLSL to SPIR-V (`.spv`)
2. **Builds C++ library** — `clang++` compiles renderer code into `librenderer.dylib`
3. **Compiles C#** — `mcs` compiles all `.cs` files into `Viewer.exe`
4. **Links at runtime** — `mono Viewer.exe` loads the `.dylib` via P/Invoke

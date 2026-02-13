# Prerequisites

## Required Tools

- **Mono** — `mcs` compiler + `mono` runtime
- **clang++** — C++ compiler with C++17 support
- **Vulkan SDK** — Vulkan headers, validation layers, and `glslc` shader compiler
- **GLFW** — windowing and input library
- **GLM** — OpenGL Mathematics library

## macOS (Homebrew)

```bash
# Install Mono
brew install mono

# Install Vulkan SDK (includes glslc, MoltenVK)
brew install --cask vulkan-sdk

# Install GLFW and GLM
brew install glfw glm
```

Verify everything is installed:

```bash
mcs --version        # Mono C# compiler
mono --version       # Mono runtime
glslc --version      # Shader compiler
brew list glfw glm   # Libraries
```

## Optional — IDE IntelliSense

For C# autocomplete, go-to-definition, and error checking in VS Code (or other IDEs), install the [.NET SDK](https://dotnet.microsoft.com/download) and run:

```bash
dotnet restore
```

This generates the project assets that the C# language server needs. The `.sln` and `.csproj` are for IDE support only — the actual build uses `mcs` via the Makefile.

## Windows & Linux

:::info Work in Progress
Windows and Linux builds are not yet supported. See the [cross-platform roadmap](../roadmap/cross-platform/windows-linux.md) for planned support.
:::

## Key Paths (macOS / Homebrew)

| Dependency   | Headers                                            | Libraries            |
| ------------ | -------------------------------------------------- | -------------------- |
| Vulkan       | `/opt/homebrew/include/vulkan/`                    | `/opt/homebrew/lib/` |
| GLFW         | `/opt/homebrew/include/GLFW/`                      | `/opt/homebrew/lib/` |
| GLM          | `/opt/homebrew/include/glm/`                       | (header-only)        |
| MoltenVK ICD | `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json` | —                    |

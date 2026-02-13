# Project Structure

## File Layout

The project uses a two-folder structure separating engine code from game code:

```
managed/                        ← ENGINE (stable, user doesn't edit)
  SaFiEngine.csproj               .NET project file (IDE IntelliSense only, not used by build)
  Viewer.cs                       Entry point (Main), main loop
  World.cs                        ECS world: entities, components, systems, queries
  Components.cs                   Built-in: Transform, MeshComponent, Movable, Camera, Light
  NativeBridge.cs                 All P/Invoke declarations + convenience wrappers
  FreeCameraState.cs              Static state for the debug free camera
  HotReload.cs                    File watcher + recompiler (dev mode only)

game_logic/                     ← GAME CODE (user edits these)
  Game.cs                         Scene setup + system registration (Game.Setup entry point)
  Systems.cs                      Per-frame system logic (InputMovement, FreeCamera, etc.)
  GameConstants.cs                Tunable config values (debug, sensitivity, speed)

native/
  renderer.h                      VulkanRenderer class declaration
  renderer.cpp                    Vulkan rendering + multi-entity API
  bridge.cpp                      extern "C" bridge functions
  shaders/
    shader.vert                   Vertex shader (UBO for view/proj, push constant for model)
    shader.frag                   Fragment shader (Blinn-Phong, up to 8 lights)
    ui.vert                       UI vertex shader (pixel-to-NDC via push constant)
    ui.frag                       UI fragment shader (R8 font atlas sampling + alpha)
  vendor/
    cgltf.h                       glTF 2.0 parsing library
    stb_truetype.h                Font rasterization library
    stb_image.h                   Image decoding library (PNG, JPEG)

assets/
  fonts/
    RobotoMono-Regular.ttf        Monospace font for debug overlay

models/                           glTF model files (.glb)
```

## Solution & Project Files

`SaFiEngine.sln` (repo root) and `managed/SaFiEngine.csproj` provide C# IntelliSense in VS Code and other IDEs. They are not used by the Makefile build. After cloning, run `dotnet restore` once to enable autocomplete and go-to-definition.

## Adding New .cs Files

The Makefile uses two separate file lists — one for engine code, one for game logic:

```makefile
MANAGED_CS = managed/Viewer.cs managed/World.cs managed/Components.cs \
             managed/NativeBridge.cs managed/FreeCameraState.cs
GAMELOGIC_CS_FILES = game_logic/Game.cs game_logic/Systems.cs game_logic/GameConstants.cs
VIEWER_CS = $(MANAGED_CS) $(GAMELOGIC_CS_FILES)
```

- **Game files** go in `game_logic/` — add the path to `GAMELOGIC_CS_FILES` in the Makefile
- **Engine files** go in `managed/` — add the path to `MANAGED_CS` in the Makefile

The `.csproj` uses `EnableDefaultCompileItems` so it discovers new files automatically — no project file changes needed. Only the Makefile needs updating.

## Adding New Components

1. Create a class in `managed/Components.cs` (or a new file in `managed/`)
2. Add fields with default values
3. If in a new file, add it to `MANAGED_CS` in the Makefile

```csharp
public class Health
{
    public float Current = 100f;
    public float Max = 100f;
}
```

## Adding New Systems

1. Add a static method to `game_logic/Systems.cs` (or a new file in `game_logic/`)
2. Register it with `world.AddSystem()` in `game_logic/Game.cs`
3. Place it before `RenderSyncSystem` in the registration order

```csharp
public static void MySystem(World world)
{
    var entities = world.Query(typeof(MyComponent));
    foreach (int e in entities)
    {
        // ...
    }
}
```

## Scene Setup

`game_logic/Game.cs` is the entry point for all game-specific setup. The engine calls `Game.Setup(world)` after initializing the renderer. This is where you spawn entities, configure components, and register systems:

```csharp
public static class Game
{
    public static void Setup(World world)
    {
        // Spawn entities, add components, register systems here
    }
}
```

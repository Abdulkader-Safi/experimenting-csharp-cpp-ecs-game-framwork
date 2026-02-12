# Project Structure

## File Layout

```
managed/
  Viewer.cs              # Entry point (Main)
  ecs/
    World.cs             # ECS world: entities, components, systems, queries
    Components.cs        # Built-in: Transform, MeshComponent, Movable, Camera, Light
    Systems.cs           # Built-in: InputMovement, FreeCamera, CameraFollow, LightSync, DebugOverlay, RenderSync
    NativeBridge.cs      # All P/Invoke declarations + convenience wrappers
    FreeCameraState.cs   # Static state for the debug free camera
    GameConstants.cs     # Tunable config values (debug, sensitivity, speed)

native/
  renderer.h             # VulkanRenderer class declaration
  renderer.cpp           # Vulkan rendering + multi-entity API
  bridge.cpp             # extern "C" bridge functions
  shaders/
    shader.vert          # Vertex shader (UBO for view/proj, push constant for model)
    shader.frag          # Fragment shader (Blinn-Phong, up to 8 lights)
    ui.vert              # UI vertex shader (pixel-to-NDC via push constant)
    ui.frag              # UI fragment shader (R8 font atlas sampling + alpha)
  vendor/
    cgltf.h              # glTF 2.0 parsing library
    stb_truetype.h       # Font rasterization library
    stb_image.h          # Image decoding library (PNG, JPEG)

assets/
  fonts/
    RobotoMono-Regular.ttf  # Monospace font for debug overlay

models/                  # glTF model files (.glb)
```

## Adding New .cs Files

When you create a new `.cs` file, add it to the `VIEWER_CS` variable in the `Makefile`:

```makefile
VIEWER_CS = managed/Viewer.cs \
    managed/ecs/World.cs \
    managed/ecs/Components.cs \
    managed/ecs/Systems.cs \
    managed/ecs/NativeBridge.cs \
    managed/ecs/MyNewFile.cs
```

## Adding New Components

1. Create a class in `managed/ecs/Components.cs` (or a new file)
2. Add fields with default values
3. If in a new file, add it to `VIEWER_CS` in the Makefile

```csharp
public class Health
{
    public float Current = 100f;
    public float Max = 100f;
}
```

## Adding New Systems

1. Add a static method to `managed/ecs/Systems.cs` (or a new file)
2. Register it with `world.AddSystem()` in `Viewer.cs`
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

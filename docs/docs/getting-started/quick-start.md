# Quick Start: Build Your First Scene

This tutorial walks you through setting up a scene with a 3D model, camera controls, and a light in `game_logic/Game.cs`.

## How It Works

The engine handles the main loop and renderer initialization in `managed/Viewer.cs`. Your game code lives in `game_logic/Game.cs`, where the engine calls `Game.Setup(world)` to let you set up your scene and register systems.

## 1. Open Game.cs

Open `game_logic/Game.cs`. This is the entry point for all your game setup:

```csharp
using System;
using ECS;

public static class Game
{
    public static void Setup(World world)
    {
        // Your scene setup goes here
    }
}
```

## 2. Load a Mesh and Spawn an Entity

Load a glTF model, create an entity, and give it a transform, mesh, and movement controls:

```csharp
        int meshId = NativeBridge.LoadMesh("models/Box.glb");

        int player = world.Spawn();
        world.AddComponent(player, new Transform());
        world.AddComponent(player, new MeshComponent {
            MeshId = meshId,
            RendererEntityId = NativeBridge.CreateEntity(meshId)
        });
        world.AddComponent(player, new Movable());
```

## 3. Add a Camera

Attach a `Camera` component to follow the player. The orbit camera lets you rotate around the entity with keyboard (Q/E/R/F) or mouse look (ESC to toggle cursor lock):

```csharp
        world.AddComponent(player, new Camera {
            OffsetZ = 3f,   // orbit distance
            Fov = 45f
        });
```

## 4. Add a Light

Create a directional light so the scene isn't dark:

```csharp
        int sun = world.Spawn();
        world.AddComponent(sun, new Transform { Y = 5f });
        world.AddComponent(sun, new Light {
            Type = Light.Directional,
            DirX = 0.2f, DirY = -1f, DirZ = 0.3f,
            Intensity = 1.0f
        });
```

## 5. Register Systems

Systems process entities each frame. Register them in order — input first, sync to GPU last:

```csharp
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.LightSyncSystem);
        world.AddSystem(Systems.RenderSyncSystem);
```

## 6. Build and Run

```bash
make run
```

The engine handles the main loop, renderer init, and cleanup — you only need to define your scene in `Game.Setup`.

## Full Code

Here's the complete `game_logic/Game.cs`:

```csharp
using System;
using ECS;

public static class Game
{
    public static void Setup(World world)
    {
        // Load mesh and create player
        int meshId = NativeBridge.LoadMesh("models/Box.glb");
        int player = world.Spawn();
        world.AddComponent(player, new Transform());
        world.AddComponent(player, new MeshComponent {
            MeshId = meshId,
            RendererEntityId = NativeBridge.CreateEntity(meshId)
        });
        world.AddComponent(player, new Movable());
        world.AddComponent(player, new Camera { OffsetZ = 3f, Fov = 45f });

        // Add a directional light
        int sun = world.Spawn();
        world.AddComponent(sun, new Transform { Y = 5f });
        world.AddComponent(sun, new Light {
            Type = Light.Directional,
            DirX = 0.2f, DirY = -1f, DirZ = 0.3f,
            Intensity = 1.0f
        });

        // Register systems (order matters!)
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.LightSyncSystem);
        world.AddSystem(Systems.RenderSyncSystem);
    }
}
```

## Controls

| Key                 | Action                    |
| ------------------- | ------------------------- |
| WASD / Arrow Keys   | Rotate the model          |
| Q / E               | Orbit camera left / right |
| R / F               | Orbit camera up / down    |
| Mouse (when locked) | Free-look camera          |
| ESC                 | Toggle cursor lock        |

## Next Steps

- [Add more entities](../ecs/entities.md) — spawn multiple objects
- [Add lights](../features/lighting.md) — point lights and spotlights
- [Write custom systems](../ecs/systems.md) — add your own game logic

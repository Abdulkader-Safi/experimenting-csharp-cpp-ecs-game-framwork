---
sidebar_position: 3
---

# Quick Start: Build Your First Scene

This tutorial walks you through creating a scene with a 3D model, camera controls, and a light — all in under 50 lines of C#.

## 1. Set Up the Renderer

Every app starts by initializing the Vulkan renderer with a window size and title:

```csharp
using System;
using ECS;

class MyApp
{
    static void Main()
    {
        NativeBridge.renderer_init(800, 600, "My First Scene");
```

## 2. Create the World

The `World` is the container for all ECS state — entities, components, and systems:

```csharp
        var world = new World();
```

## 3. Load a Mesh and Spawn an Entity

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

## 4. Add a Camera

Attach a `Camera` component to follow the player. The orbit camera lets you rotate around the entity with keyboard (Q/E/R/F) or mouse look (ESC to toggle cursor lock):

```csharp
        world.AddComponent(player, new Camera {
            OffsetZ = 3f,   // orbit distance
            Fov = 45f
        });
```

## 5. Add a Light

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

## 6. Register Systems and Run

Systems process entities each frame. Register them in order — input first, sync to GPU last:

```csharp
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.LightSyncSystem);
        world.AddSystem(Systems.RenderSyncSystem);

        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_update_time();
            NativeBridge.renderer_poll_events();
            world.DeltaTime = NativeBridge.renderer_get_delta_time();
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

        NativeBridge.renderer_cleanup();
    }
}
```

## Full Code

```csharp
using System;
using ECS;

class MyApp
{
    static void Main()
    {
        NativeBridge.renderer_init(800, 600, "My First Scene");
        var world = new World();

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

        // Main loop
        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_update_time();
            NativeBridge.renderer_poll_events();
            world.DeltaTime = NativeBridge.renderer_get_delta_time();
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

        NativeBridge.renderer_cleanup();
    }
}
```

## Controls

| Key | Action |
|---|---|
| WASD / Arrow Keys | Rotate the model |
| Q / E | Orbit camera left / right |
| R / F | Orbit camera up / down |
| Mouse (when locked) | Free-look camera |
| ESC | Toggle cursor lock |

## Next Steps

- [Add more entities](../ecs/entities.md) — spawn multiple objects
- [Add lights](../features/lighting.md) — point lights and spotlights
- [Write custom systems](../ecs/systems.md) — add your own game logic

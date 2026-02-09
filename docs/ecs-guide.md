# ECS Guide

This project uses a Bevy-inspired Entity Component System where **C# owns the game logic** (components, systems, queries) and **C++ is the rendering backend** (Vulkan, multi-entity draw calls). All ECS code lives in `managed/ecs/`.

## Architecture

```
C# (Mono)                          C++ (Vulkan)
┌─────────────────────┐            ┌──────────────────────┐
│  World              │            │  VulkanRenderer      │
│   ├─ Entities       │            │   ├─ Meshes (GPU)    │
│   ├─ Components     │  P/Invoke  │   ├─ Entities        │
│   ├─ Systems ───────┼───────────>│   └─ Push Constants  │
│   └─ Queries        │            │      (per-entity     │
│                     │            │       model matrix)  │
└─────────────────────┘            └──────────────────────┘
```

The C# `World` tracks entities and their components. Systems run each frame, querying for entities with specific component combinations. The `RenderSyncSystem` pushes transform data to the C++ renderer via `NativeBridge`.

## Prerequisites

- Mono (`mcs` compiler + `mono` runtime)
- CMake 3.20+, clang++
- Vulkan SDK, GLFW, GLM (all via Homebrew)
- `glslc` (shader compiler, comes with Vulkan SDK)

## Build & Run

```bash
make clean && make run
```

This compiles shaders, builds the C++ shared library, compiles all C# files, and launches the viewer.

To build a macOS `.app` bundle:

```bash
make app
open build/Viewer.app
```

## Quick Start

A minimal program that loads a model and renders it with WASD controls:

```csharp
using System;
using ECS;

class MyApp
{
    static void Main()
    {
        NativeBridge.renderer_init(800, 600, "My App");

        var world = new World();

        int meshId = NativeBridge.LoadMesh("models/Box.glb");

        int entity = world.Spawn();
        world.AddComponent(entity, new Transform());
        world.AddComponent(entity, new MeshComponent {
            MeshId = meshId,
            RendererEntityId = NativeBridge.CreateEntity(meshId)
        });
        world.AddComponent(entity, new Movable());
        world.AddComponent(entity, new Camera());

        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.RenderSyncSystem);

        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_poll_events();
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

        NativeBridge.renderer_cleanup();
    }
}
```

## Concepts

### World

The `World` is the container for all ECS state. Create one at startup.

```csharp
var world = new World();
```

**API:**

| Method | Description |
|---|---|
| `int Spawn()` | Create a new entity, returns its ID |
| `void Despawn(int entity)` | Destroy an entity and remove all its components |
| `void AddComponent<T>(int entity, T component)` | Attach a component to an entity |
| `T GetComponent<T>(int entity)` | Get a component from an entity (returns `null` if missing) |
| `bool HasComponent<T>(int entity)` | Check if an entity has a component |
| `List<int> Query(params Type[] types)` | Find all entities that have **all** of the given component types |
| `void AddSystem(Action<World> system)` | Register a system function |
| `void RunSystems()` | Execute all registered systems in order |

### Entities

An entity is just an integer ID. It has no data of its own -- all state is stored in components.

```csharp
int player = world.Spawn();
int enemy = world.Spawn();

// Later, to destroy:
world.Despawn(enemy);
```

### Components

A component is any C# class. It holds data, not behavior. Components must be reference types (classes, not structs).

**Built-in components** (in `managed/ecs/Components.cs`):

#### Transform

Position, rotation (degrees), and scale in 3D space.

```csharp
world.AddComponent(entity, new Transform {
    X = 1.0f, Y = 0.0f, Z = -2.0f,    // position
    RotX = 0f, RotY = 45f, RotZ = 0f,  // rotation in degrees
    ScaleX = 1f, ScaleY = 1f, ScaleZ = 1f
});
```

`Transform.ToMatrix()` returns a column-major `float[16]` matching GLM's memory layout. The rotation order is Rz * Ry * Rx, matching a `glm::rotate` chain of X then Y then Z.

#### MeshComponent

Links an ECS entity to a loaded mesh and a renderer-side entity.

```csharp
int meshId = NativeBridge.LoadMesh("models/MyModel.glb");
int rendererEntity = NativeBridge.CreateEntity(meshId);

world.AddComponent(entity, new MeshComponent {
    MeshId = meshId,
    RendererEntityId = rendererEntity
});
```

- `MeshId` -- returned by `NativeBridge.LoadMesh()`, identifies the geometry data on the GPU
- `RendererEntityId` -- returned by `NativeBridge.CreateEntity()`, identifies a draw slot in the renderer

You can create multiple entities that share the same `MeshId` (instancing the same geometry).

#### Movable

A marker component that flags an entity as controllable by WASD/arrow keys.

```csharp
world.AddComponent(entity, new Movable { Speed = 2.0f });
```

`Speed` controls how many degrees of rotation are applied per frame when a key is held.

#### Camera

Attaches an orbiting camera to an entity. The camera follows the entity's position and can be rotated with Q/E (yaw) and R/F (pitch) keyboard input, or with the mouse when the cursor is locked.

```csharp
world.AddComponent(entity, new Camera {
    OffsetX = 0f, OffsetY = 0f, OffsetZ = 3f,  // distance from entity
    Yaw = 0f, Pitch = 0f,                       // orbit angles (degrees)
    Fov = 45f,                                   // field of view (degrees)
    LookSpeed = 1.5f,                            // degrees per frame (keyboard)
    MouseSensitivity = 0.15f                     // degrees per pixel (mouse)
});
```

- `OffsetX/Y/Z` -- initial offset vector from the entity; its length determines orbit distance (default `0, 0, 3`)
- `Yaw` / `Pitch` -- orbit angles updated by keyboard and mouse input; pitch clamps to [-89, 89]
- `Fov` -- vertical field of view passed to the perspective projection
- `LookSpeed` -- how many degrees the camera orbits per frame when a key is held
- `MouseSensitivity` -- degrees of rotation per pixel of mouse movement (default `0.15`)
- `LastMouseX/Y`, `MouseInitialized`, `WasEscPressed` -- internal state used by `CameraFollowSystem` for mouse delta tracking and ESC edge-detection

If no `Camera` component is attached, the renderer uses its built-in default (eye at `(0,0,3)`, looking at origin, 45-degree FOV).

### Writing Custom Components

Define a class in the `ECS` namespace (or any namespace, as long as you reference it). Components are plain data holders.

```csharp
namespace ECS
{
    public class Health
    {
        public float Current = 100f;
        public float Max = 100f;
    }

    public class Velocity
    {
        public float VX = 0f, VY = 0f, VZ = 0f;
    }

    public class Enemy
    {
        public float AggroRange = 5f;
    }
}
```

Then attach them to entities:

```csharp
int goblin = world.Spawn();
world.AddComponent(goblin, new Transform { X = 5f });
world.AddComponent(goblin, new Health { Current = 50f, Max = 50f });
world.AddComponent(goblin, new Enemy { AggroRange = 3f });
world.AddComponent(goblin, new MeshComponent {
    MeshId = goblinMeshId,
    RendererEntityId = NativeBridge.CreateEntity(goblinMeshId)
});
```

### Systems

A system is a static method with the signature `void SystemName(World world)`. It queries for entities with specific components and acts on them.

**Built-in systems** (in `managed/ecs/Systems.cs`):

- `InputMovementSystem` -- queries `Movable + Transform`, applies WASD/arrow rotation
- `CameraFollowSystem` -- queries `Camera + Transform`, orbits the camera around the entity using Q/E/R/F keyboard input and mouse movement when the cursor is locked. ESC toggles cursor lock on/off.
- `RenderSyncSystem` -- queries `Transform + MeshComponent`, pushes `Transform.ToMatrix()` to the C++ renderer

**Registration order matters.** Systems run in the order they are added.

```csharp
world.AddSystem(Systems.InputMovementSystem);   // runs first
world.AddSystem(Systems.CameraFollowSystem);    // runs second (updates camera from input)
world.AddSystem(Systems.RenderSyncSystem);      // runs last (syncs transforms to GPU)
```

### Writing Custom Systems

```csharp
public static void GravitySystem(World world)
{
    var entities = world.Query(typeof(Transform), typeof(Velocity));
    foreach (int e in entities)
    {
        var vel = world.GetComponent<Velocity>(e);
        var tr = world.GetComponent<Transform>(e);

        vel.VY -= 9.8f * 0.016f; // gravity * dt
        tr.X += vel.VX * 0.016f;
        tr.Y += vel.VY * 0.016f;
        tr.Z += vel.VZ * 0.016f;
    }
}

public static void DespawnDeadSystem(World world)
{
    var entities = world.Query(typeof(Health));
    foreach (int e in entities)
    {
        var hp = world.GetComponent<Health>(e);
        if (hp.Current <= 0f)
        {
            // Clean up renderer entity if it has one
            var mc = world.GetComponent<MeshComponent>(e);
            if (mc != null && mc.RendererEntityId >= 0)
                NativeBridge.RemoveEntity(mc.RendererEntityId);

            world.Despawn(e);
        }
    }
}
```

Register them:

```csharp
world.AddSystem(GravitySystem);
world.AddSystem(DespawnDeadSystem);
world.AddSystem(Systems.InputMovementSystem);
world.AddSystem(Systems.CameraFollowSystem);
world.AddSystem(Systems.RenderSyncSystem);  // always last -- syncs to GPU
```

**Important:** `RenderSyncSystem` should always be the last system so it sees the final state of all transforms for the current frame.

## NativeBridge Reference

`NativeBridge` (`managed/ecs/NativeBridge.cs`) provides all P/Invoke bindings to the C++ renderer.

### Renderer Lifecycle

```csharp
NativeBridge.renderer_init(800, 600, "Window Title");  // create window + Vulkan
NativeBridge.renderer_cleanup();                        // destroy everything
```

### Main Loop

```csharp
NativeBridge.renderer_should_close();  // true when user closes window
NativeBridge.renderer_poll_events();   // process window/input events
NativeBridge.renderer_render_frame();  // draw all active entities
```

### Mesh & Entity Management

```csharp
int meshId = NativeBridge.LoadMesh("path/to/model.glb");    // load glTF, returns mesh ID
int entityId = NativeBridge.CreateEntity(meshId);            // create a draw slot
NativeBridge.SetEntityTransform(entityId, float[16] matrix); // set 4x4 model matrix
NativeBridge.RemoveEntity(entityId);                         // destroy draw slot
```

### Camera

```csharp
NativeBridge.SetCamera(eyeX, eyeY, eyeZ,       // camera position
                       targetX, targetY, targetZ, // look-at point
                       upX, upY, upZ,             // up vector
                       fovDegrees);                // vertical FOV
```

Sets the view and projection matrices for rendering. If never called, defaults to eye `(0,0,3)`, target `(0,0,0)`, up `(0,1,0)`, FOV `45`. Typically called by `CameraFollowSystem` rather than directly.

### Input

```csharp
bool pressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W);
```

Available key constants: `GLFW_KEY_W`, `GLFW_KEY_A`, `GLFW_KEY_S`, `GLFW_KEY_D`, `GLFW_KEY_Q`, `GLFW_KEY_E`, `GLFW_KEY_R`, `GLFW_KEY_F`, `GLFW_KEY_ESCAPE`, `GLFW_KEY_UP`, `GLFW_KEY_DOWN`, `GLFW_KEY_LEFT`, `GLFW_KEY_RIGHT`. For other keys, pass the [GLFW key code](https://www.glfw.org/docs/latest/group__keys.html) integer directly.

### Cursor

```csharp
NativeBridge.SetCursorLocked(true);   // capture cursor (hides it, enables raw mouse input)
NativeBridge.SetCursorLocked(false);  // release cursor
bool locked = NativeBridge.IsCursorLocked();

double mx, my;
NativeBridge.GetCursorPos(out mx, out my);  // current cursor position in window coordinates
```

Cursor locking is used by `CameraFollowSystem` for mouse look. Press ESC to toggle.

## Adding New .cs Files to the Build

When you create a new `.cs` file, add it to the `VIEWER_CS` variable in the `Makefile`:

```makefile
VIEWER_CS = managed/Viewer.cs \
    managed/ecs/World.cs \
    managed/ecs/Components.cs \
    managed/ecs/Systems.cs \
    managed/ecs/NativeBridge.cs \
    managed/ecs/MyNewFile.cs
```

## File Structure

```
managed/
  Viewer.cs              # Entry point (Main)
  ecs/
    World.cs             # ECS world: entities, components, systems, queries
    Components.cs        # Built-in components: Transform, MeshComponent, Movable, Camera
    Systems.cs           # Built-in systems: InputMovementSystem, CameraFollowSystem, RenderSyncSystem
    NativeBridge.cs      # All P/Invoke declarations + convenience wrappers

native/
  renderer.h             # VulkanRenderer class declaration
  renderer.cpp           # Vulkan rendering + multi-entity API
  bridge.cpp             # extern "C" bridge functions
  shaders/
    shader.vert          # Vertex shader (UBO for view/proj, push constant for model)
    shader.frag          # Fragment shader (diffuse + ambient lighting)
```

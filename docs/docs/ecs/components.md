# Components

A component is any C# class that holds data, not behavior. Components must be reference types (classes, not structs). All built-in components live in `managed/Components.cs`.

## Built-in Components

### Transform

Position, rotation (degrees), and scale in 3D space.

```csharp
world.AddComponent(entity, new Transform {
    X = 1.0f, Y = 0.0f, Z = -2.0f,    // position
    RotX = 0f, RotY = 45f, RotZ = 0f,  // rotation in degrees
    ScaleX = 1f, ScaleY = 1f, ScaleZ = 1f
});
```

`Transform.ToMatrix()` returns a column-major `float[16]` matching GLM's memory layout. The rotation order is Rz _ Ry _ Rx, matching a `glm::rotate` chain of X then Y then Z.

### MeshComponent

Links an ECS entity to a loaded mesh and a renderer-side entity.

```csharp
int meshId = NativeBridge.LoadMesh("models/MyModel.glb");
int rendererEntity = NativeBridge.CreateEntity(meshId);

world.AddComponent(entity, new MeshComponent {
    MeshId = meshId,
    RendererEntityId = rendererEntity
});
```

| Field              | Description                                                                       |
| ------------------ | --------------------------------------------------------------------------------- |
| `MeshId`           | Returned by `NativeBridge.LoadMesh()`, identifies the geometry data on the GPU    |
| `RendererEntityId` | Returned by `NativeBridge.CreateEntity()`, identifies a draw slot in the renderer |

You can create multiple entities sharing the same `MeshId` (instancing the same geometry).

### Movable

A marker component that flags an entity as controllable by WASD/arrow keys.

```csharp
world.AddComponent(entity, new Movable { Speed = 2.0f });
```

`Speed` controls how many degrees of rotation are applied per second when a key is held (multiplied by `DeltaTime`).

### Camera

Attaches an orbiting camera to an entity. See the [Camera feature page](../features/camera.md) for full details.

```csharp
world.AddComponent(entity, new Camera {
    OffsetX = 0f, OffsetY = 0f, OffsetZ = 3f,
    Yaw = 0f, Pitch = 0f,
    Fov = 45f,
    LookSpeed = 90f,
    MouseSensitivity = 0.15f
});
```

| Field                  | Description                                                                          |
| ---------------------- | ------------------------------------------------------------------------------------ |
| `OffsetX/Y/Z`          | Initial offset vector; its length determines orbit distance (default `0, 0, 3`)      |
| `Yaw` / `Pitch`        | Orbit angles updated by keyboard and mouse input; pitch clamps to [-89, 89]          |
| `Fov`                  | Vertical field of view in degrees                                                    |
| `LookSpeed`            | Degrees per second for keyboard orbit (Q/E/R/F)                                      |
| `MouseSensitivity`     | Degrees per pixel of mouse movement (default `0.15`)                                 |
| `Mode`                 | `0` = third-person orbit (default), `1` = first-person                               |
| `EyeHeight`            | Vertical offset above entity position for first-person eye placement (default `0.8`) |
| `WasModeTogglePressed` | Internal state for edge-detecting TAB toggle                                         |
| `MinDistance`          | Minimum zoom distance in third-person mode (default `1`)                             |
| `MaxDistance`          | Maximum zoom distance in third-person mode (default `20`)                            |
| `ZoomSpeed`            | Scroll wheel zoom sensitivity (default `2`)                                          |

### Timer

A countdown or interval timer. Ticked automatically by `TimerSystem`.

```csharp
world.AddComponent(entity, new Timer {
    Duration = 2.0f,
    Repeat = true,
    Tag = "spawn"
});
```

| Field      | Description                                                             |
| ---------- | ----------------------------------------------------------------------- |
| `Duration` | Time in seconds until the timer fires (default `1`)                     |
| `Elapsed`  | Current elapsed time, managed by `TimerSystem`                          |
| `Repeat`   | If `true`, resets and fires again each interval; if `false`, fires once |
| `Finished` | Set to `true` when elapsed reaches duration                             |
| `Tag`      | Optional string label to identify the timer's purpose                   |

### Hierarchy

Links a child entity to its parent for transform inheritance and cascade despawn.

```csharp
world.AddComponent(child, new Hierarchy { Parent = parentEntity });
```

| Field    | Description                                           |
| -------- | ----------------------------------------------------- |
| `Parent` | Entity ID of the parent, or `-1` for root (no parent) |

### WorldTransform

Stores the computed world-space transform matrix for entities in a hierarchy. Managed automatically by `HierarchyTransformSystem`.

| Field    | Description                                               |
| -------- | --------------------------------------------------------- |
| `Matrix` | Column-major `float[16]` world-space 4x4 transform matrix |

### Light

A dynamic light source. See the [Lighting feature page](../features/lighting.md) for full details.

```csharp
world.AddComponent(entity, new Light {
    Type = Light.Directional,
    ColorR = 1f, ColorG = 1f, ColorB = 1f,
    Intensity = 1f,
    DirX = 0f, DirY = -1f, DirZ = 0f,
    Radius = 10f,
    InnerConeDeg = 12.5f, OuterConeDeg = 17.5f
});
```

| Field                           | Description                                                     |
| ------------------------------- | --------------------------------------------------------------- |
| `Type`                          | `Light.Directional` (0), `Light.Point` (1), or `Light.Spot` (2) |
| `ColorR/G/B`                    | Light color (default white)                                     |
| `Intensity`                     | Brightness multiplier                                           |
| `DirX/Y/Z`                      | Direction vector (for directional and spot lights)              |
| `Radius`                        | Attenuation radius (for point and spot lights)                  |
| `InnerConeDeg` / `OuterConeDeg` | Cone angles in degrees (for spot lights)                        |
| `LightIndex`                    | Assigned automatically by `LightSyncSystem`                     |

### Rigidbody

Defines the dynamics properties of a physics body. See the [Physics feature page](../features/physics.md) for full details.

```csharp
world.AddComponent(entity, new Rigidbody {
    MotionType = JPH_MotionType.Dynamic,
    Friction = 0.5f,
    Restitution = 0.3f,
    LinearDamping = 0.05f,
    AngularDamping = 0.05f,
    GravityFactor = 1.0f
});
```

| Field            | Description                                                          |
| ---------------- | -------------------------------------------------------------------- |
| `MotionType`     | `JPH_MotionType.Static`, `Kinematic`, or `Dynamic` (default Dynamic) |
| `Friction`       | Surface friction coefficient (default `0.5`)                          |
| `Restitution`    | Bounciness (default `0.3`)                                            |
| `LinearDamping`  | Velocity decay per second (default `0.05`)                            |
| `AngularDamping` | Angular velocity decay per second (default `0.05`)                    |
| `GravityFactor`  | Gravity multiplier (default `1.0`)                                    |
| `BodyId`         | Assigned by Jolt after body creation                                  |
| `BodyCreated`    | Set to `true` once the physics body exists                            |

### Color

A utility class for representing RGBA colors. Used by `Collider.DebugColor` and available for custom components.

```csharp
// From float values (alpha defaults to 1.0)
new Color(1f, 0.5f, 0f)
new Color(1f, 0f, 0f, 0.5f)  // with alpha

// From hex string (#RRGGBB or #RRGGBBAA)
new Color("#ff6600")
new Color("#ff660080")  // with alpha

// Presets
Color.Green   // (0, 1, 0)
Color.Red     // (1, 0, 0)
Color.Blue    // (0, 0, 1)
Color.Yellow  // (1, 1, 0)
Color.Cyan    // (0, 1, 1)
Color.White   // (1, 1, 1)
```

| Field | Description                      |
| ----- | -------------------------------- |
| `R`   | Red channel (0.0 - 1.0)         |
| `G`   | Green channel (0.0 - 1.0)       |
| `B`   | Blue channel (0.0 - 1.0)        |
| `A`   | Alpha channel (0.0 - 1.0)       |

### Collider

Defines the collision shape. Used together with `Rigidbody` and `Transform`.

```csharp
world.AddComponent(entity, new Collider {
    ShapeType = Collider.Box,
    BoxHalfX = 0.5f, BoxHalfY = 0.5f, BoxHalfZ = 0.5f,
    DebugColor = Color.Red
});
```

| Field             | Description                                                     |
| ----------------- | --------------------------------------------------------------- |
| `ShapeType`       | `Collider.Box` (0), `Sphere` (1), `Capsule` (2), `Cylinder` (3), `Plane` (4) |
| `BoxHalfX/Y/Z`   | Half-extents for box shape (default `0.5`)                      |
| `SphereRadius`    | Radius for sphere shape (default `0.5`)                         |
| `CapsuleHalfHeight/Radius` | Capsule dimensions (default `0.5` / `0.3`)             |
| `CylinderHalfHeight/Radius` | Cylinder dimensions (default `0.5` / `0.5`)           |
| `PlaneNormalX/Y/Z` | Plane normal direction (default `0, 1, 0`)                    |
| `PlaneDistance`   | Plane offset along normal (default `0`)                         |
| `PlaneHalfExtent` | Plane size (default `100`)                                     |
| `DebugColor`      | Wireframe color for debug overlay (default `Color.Green`). Accepts `Color` presets, float values, or hex strings |

## Writing Custom Components

Define a class — components are plain data holders:

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

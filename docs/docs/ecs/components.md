# Components

A component is any C# class that holds data, not behavior. Components must be reference types (classes, not structs). All built-in components live in `managed/Components.cs`.

## Utility Types

### Vec3

A 3D vector used for positions, rotations, directions, and sizes throughout the API.

```csharp
new Vec3()                // (0, 0, 0)
new Vec3(1f, 2f, 3f)     // (1, 2, 3)
```

| Field | Description         |
| ----- | ------------------- |
| `X`   | X component (float) |
| `Y`   | Y component (float) |
| `Z`   | Z component (float) |

### Color

A utility class for representing RGBA colors. Used by `Light.LightColor`, `Collider.DebugColor`, and procedural mesh creation.

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

| Field | Description               |
| ----- | ------------------------- |
| `R`   | Red channel (0.0 - 1.0)   |
| `G`   | Green channel (0.0 - 1.0) |
| `B`   | Blue channel (0.0 - 1.0)  |
| `A`   | Alpha channel (0.0 - 1.0) |

## Enums

### LightType

```csharp
LightType.Directional  // 0 — parallel light (sun)
LightType.Point        // 1 — omnidirectional light
LightType.Spot         // 2 — cone-shaped light
```

### ShapeType

```csharp
ShapeType.Box       // 0
ShapeType.Sphere    // 1
ShapeType.Capsule   // 2
ShapeType.Cylinder  // 3
ShapeType.Plane     // 4
```

### CameraMode

```csharp
CameraMode.ThirdPerson  // 0 — orbit camera (default)
CameraMode.FirstPerson  // 1 — eye-level camera
```

## Built-in Components

### Transform

Position, rotation (degrees), and scale in 3D space.

```csharp
world.AddComponent(entity, new Transform {
    Position = new Vec3(1f, 0f, -2f),
    Rotation = new Vec3(0f, 45f, 0f),
    Scale = new Vec3(1f, 1f, 1f)
});
```

| Field      | Type   | Default     | Description             |
| ---------- | ------ | ----------- | ----------------------- |
| `Position` | `Vec3` | `(0, 0, 0)` | World-space position    |
| `Rotation` | `Vec3` | `(0, 0, 0)` | Euler angles in degrees |
| `Scale`    | `Vec3` | `(1, 1, 1)` | Per-axis scale          |

`Transform.ToMatrix()` returns a column-major `float[16]` matching GLM's memory layout. The rotation order is Rz _ Ry _ Rx, matching a `glm::rotate` chain of X then Y then Z.

### MeshComponent

Links an ECS entity to a loaded mesh and a renderer-side entity. Both fields are engine-internal (prefixed with `_`) — use `world.SpawnMeshEntity()` instead of setting them directly.

```csharp
// Preferred: use SpawnMeshEntity
int entity = world.SpawnMeshEntity(meshId, new Transform());

// Manual (advanced): fields are engine-internal
var mc = new MeshComponent();
// mc._MeshId and mc._RendererEntityId are set by SpawnMeshEntity
```

| Field               | Description                                                                       |
| ------------------- | --------------------------------------------------------------------------------- |
| `_MeshId`           | Returned by `NativeBridge.LoadMesh()`, identifies the geometry data on the GPU    |
| `_RendererEntityId` | Returned by `NativeBridge.CreateEntity()`, identifies a draw slot in the renderer |

You can create multiple entities sharing the same mesh (instancing the same geometry).

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
    Offset = new Vec3(0f, 0f, 3f),
    Yaw = 0f, Pitch = 0f,
    Fov = 45f,
    LookSpeed = 90f,
    MouseSensitivity = 0.15f
});
```

| Field              | Description                                                                              |
| ------------------ | ---------------------------------------------------------------------------------------- |
| `Offset`           | Initial offset vector (`Vec3`); its length determines orbit distance (default `0, 0, 3`) |
| `Yaw` / `Pitch`    | Orbit angles updated by keyboard and mouse input; pitch clamps to [-89, 89]              |
| `Fov`              | Vertical field of view in degrees                                                        |
| `LookSpeed`        | Degrees per second for keyboard orbit (Q/E/R/F)                                          |
| `MouseSensitivity` | Degrees per pixel of mouse movement (default `0.15`)                                     |
| `Mode`             | `CameraMode.ThirdPerson` (default) or `CameraMode.FirstPerson`                           |
| `EyeHeight`        | Vertical offset above entity position for first-person eye placement (default `0.8`)     |
| `MinDistance`      | Minimum zoom distance in third-person mode (default `1`)                                 |
| `MaxDistance`      | Maximum zoom distance in third-person mode (default `20`)                                |
| `ZoomSpeed`        | Scroll wheel zoom sensitivity (default `2`)                                              |

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
    Type = LightType.Directional,
    LightColor = Color.White,
    Intensity = 1f,
    Direction = new Vec3(0f, -1f, 0f),
    Radius = 10f,
    InnerConeDeg = 12.5f, OuterConeDeg = 17.5f
});
```

| Field                           | Description                                                     |
| ------------------------------- | --------------------------------------------------------------- |
| `Type`                          | `LightType.Directional`, `LightType.Point`, or `LightType.Spot` |
| `LightColor`                    | Light color as a `Color` (default `Color.White`)                |
| `Intensity`                     | Brightness multiplier                                           |
| `Direction`                     | Direction vector as `Vec3` (for directional and spot lights)    |
| `Radius`                        | Attenuation radius (for point and spot lights)                  |
| `InnerConeDeg` / `OuterConeDeg` | Cone angles in degrees (for spot lights)                        |
| `_LightIndex`                   | Assigned automatically by `LightSyncSystem` (engine-internal)   |

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
| `Friction`       | Surface friction coefficient (default `0.5`)                         |
| `Restitution`    | Bounciness (default `0.3`)                                           |
| `LinearDamping`  | Velocity decay per second (default `0.05`)                           |
| `AngularDamping` | Angular velocity decay per second (default `0.05`)                   |
| `GravityFactor`  | Gravity multiplier (default `1.0`)                                   |
| `_BodyId`        | Assigned by Jolt after body creation (engine-internal)               |
| `_BodyCreated`   | Set to `true` once the physics body exists (engine-internal)         |

### Collider

Defines the collision shape. Used together with `Rigidbody` and `Transform`.

```csharp
world.AddComponent(entity, new Collider {
    Shape = ShapeType.Box,
    BoxHalfExtents = new Vec3(0.5f, 0.5f, 0.5f),
    DebugColor = Color.Red
});
```

| Field                       | Description                                                                                                      |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| `Shape`                     | `ShapeType.Box`, `Sphere`, `Capsule`, `Cylinder`, or `Plane`                                                     |
| `BoxHalfExtents`            | Half-extents for box shape as `Vec3` (default `0.5, 0.5, 0.5`)                                                   |
| `SphereRadius`              | Radius for sphere shape (default `0.5`)                                                                          |
| `CapsuleHalfHeight/Radius`  | Capsule dimensions (default `0.5` / `0.3`)                                                                       |
| `CylinderHalfHeight/Radius` | Cylinder dimensions (default `0.5` / `0.5`)                                                                      |
| `PlaneNormal`               | Plane normal direction as `Vec3` (default `0, 1, 0`)                                                             |
| `PlaneDistance`             | Plane offset along normal (default `0`)                                                                          |
| `PlaneHalfExtent`           | Plane size (default `100`)                                                                                       |
| `DebugColor`                | Wireframe color for debug overlay (default `Color.Green`). Accepts `Color` presets, float values, or hex strings |

## Internal Fields (Underscore Convention)

Fields prefixed with `_` are engine-managed and should not be set directly by game code:

| Component       | Internal Fields                                                                              |
| --------------- | -------------------------------------------------------------------------------------------- |
| `Rigidbody`     | `_BodyId`, `_BodyCreated`                                                                    |
| `MeshComponent` | `_MeshId`, `_RendererEntityId`                                                               |
| `Light`         | `_LightIndex`                                                                                |
| `Camera`        | `_LastMouseX`, `_LastMouseY`, `_MouseInitialized`, `_WasEscPressed`, `_WasModeTogglePressed` |

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
        public Vec3 Value = new Vec3();
    }

    public class Enemy
    {
        public float AggroRange = 5f;
    }
}
```

Then attach them to entities:

```csharp
int goblin = world.Spawn(
    new Transform { Position = new Vec3(5f, 0f, 0f) },
    new Health { Current = 50f, Max = 50f },
    new Enemy { AggroRange = 3f }
);
world.AddComponent(goblin, new MeshComponent()); // or use SpawnMeshEntity
```

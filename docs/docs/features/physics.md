# Physics

The engine integrates [Jolt Physics](https://github.com/jrouwe/JoltPhysics) via [joltc](https://github.com/amerkoleci/joltc) -- a C API wrapper called from C# via P/Invoke. This provides rigid body dynamics, collision detection, and multiple collider shapes.

## Architecture

```
C# PhysicsWorld (managed/)         joltc (native/joltc/)
  Singleton lifecycle manager         C API wrapper around Jolt Physics
  Body tracking (entity-to-body)      Builds as libjoltc.dylib
  Fixed timestep accumulator
  PhysicsBridge.cs ──DllImport──> libjoltc.dylib
```

`PhysicsWorld` is an engine-layer singleton that survives hot reloads. `PhysicsSystem` (in `game_logic/Systems.cs`) is hot-reloadable, so you can tune physics parameters live with `make dev`.

## Components

### Rigidbody

Defines the dynamics properties of a physics body.

```csharp
world.AddComponent(entity, new Rigidbody {
    MotionType = JPH_MotionType.Dynamic,  // Static, Kinematic, or Dynamic
    Friction = 0.5f,
    Restitution = 0.3f,
    LinearDamping = 0.05f,
    AngularDamping = 0.05f,
    GravityFactor = 1.0f
});
```

| Field            | Default   | Description                                                         |
| ---------------- | --------- | ------------------------------------------------------------------- |
| `MotionType`     | `Dynamic` | `Static` (immovable), `Kinematic` (scripted), `Dynamic`             |
| `Friction`       | `0.5`     | Surface friction coefficient                                        |
| `Restitution`    | `0.3`     | Bounciness (0 = no bounce, 1 = perfectly elastic)                   |
| `LinearDamping`  | `0.05`    | Velocity decay per second                                           |
| `AngularDamping` | `0.05`    | Angular velocity decay per second                                   |
| `GravityFactor`  | `1.0`     | Gravity multiplier (0 = no gravity, 2 = double gravity)             |
| `_BodyId`        | `0`       | Set automatically after Jolt body creation (engine-internal)        |
| `_BodyCreated`   | `false`   | Set to `true` once the Jolt body has been created (engine-internal) |

### Collider

Defines the shape used for collision detection.

```csharp
// Box collider
world.AddComponent(entity, new Collider {
    Shape = ShapeType.Box,
    BoxHalfExtents = new Vec3(0.5f, 0.5f, 0.5f)
});

// Sphere collider
world.AddComponent(entity, new Collider {
    Shape = ShapeType.Sphere,
    SphereRadius = 0.5f
});

// Infinite ground plane
world.AddComponent(entity, new Collider {
    Shape = ShapeType.Plane,
    PlaneHalfExtent = 100f
});
```

| Shape    | Enum Value           | Parameters                                               |
| -------- | -------------------- | -------------------------------------------------------- |
| Box      | `ShapeType.Box`      | `BoxHalfExtents` (Vec3, half-extents)                    |
| Sphere   | `ShapeType.Sphere`   | `SphereRadius`                                           |
| Capsule  | `ShapeType.Capsule`  | `CapsuleHalfHeight`, `CapsuleRadius`                     |
| Cylinder | `ShapeType.Cylinder` | `CylinderHalfHeight`, `CylinderRadius`                   |
| Plane    | `ShapeType.Plane`    | `PlaneNormal` (Vec3), `PlaneDistance`, `PlaneHalfExtent` |

## PhysicsSystem

The `PhysicsSystem` runs three phases each frame:

1. **Create bodies** -- Finds entities with `Rigidbody` + `Collider` + `Transform` where the Jolt body hasn't been created yet. Creates the shape and body in Jolt.
2. **Step physics** -- Advances the Jolt simulation using a fixed timestep accumulator (1/60s, max 4 steps per frame).
3. **Sync transforms** -- Reads position and rotation back from Jolt for dynamic bodies and writes them to the ECS `Transform` component.

## Fixed Timestep

Physics runs at a fixed 1/60s (60 Hz) rate regardless of the rendering frame rate. The `PhysicsWorld` uses an accumulator pattern:

- Each frame, `DeltaTime` is added to the accumulator
- While the accumulator has enough time for a physics step, Jolt is stepped at 1/60s
- Maximum 4 steps per frame to prevent spiral-of-death on frame drops
- If the accumulator exceeds the maximum, it resets to zero

## Usage Example

```csharp
// Static ground plane
int ground = world.Spawn(
    new Transform { Position = new Vec3(0f, -1f, 0f) },
    new Rigidbody { MotionType = JPH_MotionType.Static, Friction = 0.8f },
    new Collider { Shape = ShapeType.Plane, PlaneHalfExtent = 100f }
);

// Dynamic box that falls under gravity
int boxMesh = NativeBridge.CreateBoxMesh(1f, 1f, 1f, new Color(0.9f, 0.2f, 0.2f));
int box = world.SpawnMeshEntity(boxMesh, new Transform { Position = new Vec3(0f, 5f, 0f) });
world.AddComponent(box, new Rigidbody { Friction = 0.5f, Restitution = 0.3f });
world.AddComponent(box, new Collider {
    Shape = ShapeType.Box,
    BoxHalfExtents = new Vec3(0.5f, 0.5f, 0.5f)
});
```

:::tip
Call `PhysicsWorld.Instance.OptimizeBroadPhase()` after spawning your initial batch of bodies for better collision detection performance.
:::

## Cleanup

Physics bodies are automatically cleaned up:

- **Despawning** an entity calls `PhysicsWorld.Instance.RemoveBody(entity)`
- **World reset** (hot reload) calls `PhysicsWorld.Instance.RemoveAllBodies()` before despawning entities
- **Shutdown** calls `PhysicsWorld.Instance.Shutdown()` before renderer cleanup

## Debug Visualization

Press **F3** to toggle the debug overlay, which draws wireframe outlines around all physics colliders. Each collider's wireframe color is configurable via the `DebugColor` field (defaults to green). This makes it easy to verify collider placement, size, and alignment with visual meshes. See [Debug Overlay](./debug-overlay.md) for details.

```csharp
new Collider { Shape = ShapeType.Box, DebugColor = Color.Red }
new Collider { Shape = ShapeType.Capsule, DebugColor = new Color("#00ffcc") }
```

Supported shapes: box, sphere, capsule, cylinder. Plane colliders are skipped (too large to render meaningfully).

## System Registration

Register `PhysicsSystem` after input/timer systems but before camera and rendering:

```csharp
world.AddSystem(Systems.InputMovementSystem);
world.AddSystem(Systems.TimerSystem);
world.AddSystem(Systems.PhysicsSystem);        // physics after input, before camera
world.AddSystem(Systems.FreeCameraSystem);
world.AddSystem(Systems.CameraFollowSystem);
// ... remaining systems ...
world.AddSystem(Systems.DebugColliderRenderSystem); // wireframe collider vis (before RenderSync)
world.AddSystem(Systems.RenderSyncSystem);     // always last
```

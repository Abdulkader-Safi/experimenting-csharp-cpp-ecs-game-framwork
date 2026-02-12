# Systems

A system is a static method with the signature `void SystemName(World world)`. It queries for entities with specific components and acts on them.

## Built-in Systems

All built-in systems live in `managed/ecs/Systems.cs`.

### InputMovementSystem

Queries: `Movable` + `Transform`

Applies WASD/arrow key rotation to movable entities. Movement speed is multiplied by `world.DeltaTime` for frame-independent rotation. **Disabled when the free camera is active** — returns early so WASD only controls the free camera.

### TimerSystem

Queries: `Timer`

Ticks all `Timer` components each frame using `world.DeltaTime`:

- **One-shot timers** (`Repeat = false`): `Finished` is set to `true` once and the timer stops advancing
- **Interval timers** (`Repeat = true`): `Finished` is set to `true` each cycle, and `Elapsed` wraps by subtracting `Duration`

### FreeCameraSystem

No query — uses static `FreeCameraState`.

A debug fly camera for inspecting the scene freely. Only activates when `GameConstants.Debug` is `true`. Press **0** to activate, **1** to deactivate.

When active:

- **WASD** — fly forward/back/strafe left/right
- **Q/E** — fly down/up
- **Mouse** — look around (when cursor is locked)
- **ESC** — toggle cursor lock

Sensitivity and speed come from `GameConstants.FreeCamSensitivity` and `GameConstants.FreeCamSpeed`. Overrides the camera directly via `NativeBridge.SetCamera()`. Both `InputMovementSystem` and `CameraFollowSystem` return early when the free camera is active.

### CameraFollowSystem

Queries: `Camera` + `Transform`

**Disabled when the free camera is active** — returns early.

Handles both camera modes:

- **Third-person** (Mode 0): Orbits the camera around the entity. Scroll wheel zooms in/out, clamped between `MinDistance` and `MaxDistance`.
- **First-person** (Mode 1): Places the camera at entity position + `EyeHeight`, looking along yaw/pitch direction.

Controls:

- **Q/E** — yaw (horizontal orbit)
- **R/F** — pitch (vertical orbit)
- **Mouse** — free-look when cursor is locked
- **Scroll wheel** — zoom (third-person only)
- **TAB** — toggle first-person / third-person (edge-detected)
- **ESC** — toggle cursor lock on/off

### HierarchyTransformSystem

Queries: `Transform`

Computes world-space transforms for entities in a parent-child hierarchy:

1. For child entities (with `Hierarchy` component pointing to a valid parent), walks the parent chain and multiplies `parent.WorldTransform * child.LocalTransform`
2. Automatically adds a `WorldTransform` component to child entities if not present
3. Root entities keep their local transform as-is

### LightSyncSystem

Queries: `Light` + `Transform`

Pushes light data to the C++ renderer each frame. Supports up to 8 active lights. Automatically assigns light slots and clears unused ones.

### DebugOverlaySystem

No query — reads key state and calls `NativeBridge.SetDebugOverlay()`.

Toggles the debug overlay on/off when the **F3** key is pressed (edge-detected). Each press flips `GameConstants.Debug` and syncs the state to the C++ renderer. See [Debug Overlay](../features/debug-overlay.md) for details on what the overlay displays.

### RenderSyncSystem

Queries: `Transform` + `MeshComponent`

Pushes transform matrices to the C++ renderer for each entity with a mesh. Uses `WorldTransform.Matrix` if available (for entities in a hierarchy), falling back to `Transform.ToMatrix()` for root entities. **This should always be the last system** so it sees the final state of all transforms.

## Registration Order

Systems run in the order they are added. **Order matters.**

```csharp
world.AddSystem(Systems.InputMovementSystem);        // runs first
world.AddSystem(Systems.TimerSystem);                 // tick timers
world.AddSystem(Systems.FreeCameraSystem);            // debug fly camera (before CameraFollow)
world.AddSystem(Systems.CameraFollowSystem);          // updates camera from input
world.AddSystem(Systems.LightSyncSystem);             // syncs lights to GPU
world.AddSystem(Systems.HierarchyTransformSystem);    // compute world transforms
world.AddSystem(Systems.DebugOverlaySystem);          // F3 toggle, syncs overlay state
world.AddSystem(Systems.RenderSyncSystem);            // runs last — syncs transforms
```

## Writing Custom Systems

```csharp
public static void GravitySystem(World world)
{
    var entities = world.Query(typeof(Transform), typeof(Velocity));
    foreach (int e in entities)
    {
        var vel = world.GetComponent<Velocity>(e);
        var tr = world.GetComponent<Transform>(e);

        vel.VY -= 9.8f * world.DeltaTime;
        tr.X += vel.VX * world.DeltaTime;
        tr.Y += vel.VY * world.DeltaTime;
        tr.Z += vel.VZ * world.DeltaTime;
    }
}

public static void DespawnDeadSystem(World world)
{
    var entities = world.Query(typeof(Health));
    foreach (int e in entities)
    {
        var hp = world.GetComponent<Health>(e);
        if (hp.Current <= 0f)
            world.Despawn(e);  // auto-cleans native renderer entity
    }
}
```

Register them before `RenderSyncSystem`:

```csharp
world.AddSystem(GravitySystem);
world.AddSystem(DespawnDeadSystem);
world.AddSystem(Systems.InputMovementSystem);
world.AddSystem(Systems.TimerSystem);
world.AddSystem(Systems.FreeCameraSystem);
world.AddSystem(Systems.CameraFollowSystem);
world.AddSystem(Systems.LightSyncSystem);
world.AddSystem(Systems.HierarchyTransformSystem);
world.AddSystem(Systems.DebugOverlaySystem);
world.AddSystem(Systems.RenderSyncSystem);  // always last
```

:::tip
`RenderSyncSystem` should always be the last system so it sees the final state of all transforms for the current frame.
:::

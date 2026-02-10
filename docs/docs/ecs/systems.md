---
sidebar_position: 4
---

# Systems

A system is a static method with the signature `void SystemName(World world)`. It queries for entities with specific components and acts on them.

## Built-in Systems

All built-in systems live in `managed/ecs/Systems.cs`.

### InputMovementSystem

Queries: `Movable` + `Transform`

Applies WASD/arrow key rotation to movable entities. Movement speed is multiplied by `world.DeltaTime` for frame-independent rotation.

### CameraFollowSystem

Queries: `Camera` + `Transform`

Orbits the camera around the entity using:
- **Q/E** — yaw (horizontal orbit)
- **R/F** — pitch (vertical orbit)
- **Mouse** — free-look when cursor is locked
- **ESC** — toggle cursor lock on/off

### LightSyncSystem

Queries: `Light` + `Transform`

Pushes light data to the C++ renderer each frame. Supports up to 8 active lights. Automatically assigns light slots and clears unused ones.

### RenderSyncSystem

Queries: `Transform` + `MeshComponent`

Pushes `Transform.ToMatrix()` to the C++ renderer for each entity with a mesh. **This should always be the last system** so it sees the final state of all transforms.

## Registration Order

Systems run in the order they are added. **Order matters.**

```csharp
world.AddSystem(Systems.InputMovementSystem);   // runs first
world.AddSystem(Systems.CameraFollowSystem);    // updates camera from input
world.AddSystem(Systems.LightSyncSystem);       // syncs lights to GPU
world.AddSystem(Systems.RenderSyncSystem);      // runs last — syncs transforms
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
        {
            var mc = world.GetComponent<MeshComponent>(e);
            if (mc != null && mc.RendererEntityId >= 0)
                NativeBridge.RemoveEntity(mc.RendererEntityId);

            world.Despawn(e);
        }
    }
}
```

Register them before `RenderSyncSystem`:

```csharp
world.AddSystem(GravitySystem);
world.AddSystem(DespawnDeadSystem);
world.AddSystem(Systems.InputMovementSystem);
world.AddSystem(Systems.CameraFollowSystem);
world.AddSystem(Systems.LightSyncSystem);
world.AddSystem(Systems.RenderSyncSystem);  // always last
```

:::tip
`RenderSyncSystem` should always be the last system so it sees the final state of all transforms for the current frame.
:::

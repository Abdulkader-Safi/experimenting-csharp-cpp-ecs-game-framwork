# Spawn System

:::tip Implemented
This feature is fully implemented.
:::

Create mesh entities at runtime and despawn them with automatic native cleanup.

## SpawnMeshEntity

`World.SpawnMeshEntity()` is a convenience method that creates an ECS entity, attaches a `Transform` and `MeshComponent`, and registers it with the native renderer in one call:

```csharp
int bullet = world.SpawnMeshEntity(bulletMeshId, new Transform {
    X = spawnX, Y = spawnY, Z = spawnZ
});
```

This is equivalent to:

```csharp
int entity = world.Spawn();
world.AddComponent(entity, transform);
world.AddComponent(entity, new MeshComponent {
    MeshId = meshId,
    RendererEntityId = NativeBridge.CreateEntity(meshId)
});
```

## Despawn with Auto-Cleanup

`World.Despawn()` automatically cleans up native renderer entities — no manual `RemoveEntity` call needed:

```csharp
world.Despawn(entity);  // removes ECS entity + native renderer entity + cascade-deletes children
```

- If the entity has a `MeshComponent`, its `RendererEntityId` is passed to `NativeBridge.RemoveEntity()` before removal
- If the entity has children (via `Hierarchy`), they are recursively despawned as well

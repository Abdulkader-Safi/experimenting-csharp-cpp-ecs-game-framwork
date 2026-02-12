# Entities

An entity is just an integer ID. It has no data of its own — all state is stored in [components](./components.md).

## Creating Entities

```csharp
int player = world.Spawn();
int enemy = world.Spawn();
```

Each call to `Spawn()` returns a unique integer ID.

### SpawnMeshEntity

For entities that need a mesh, `SpawnMeshEntity` creates the entity, attaches a `Transform` and `MeshComponent`, and registers it with the native renderer in one call:

```csharp
int meshId = NativeBridge.LoadMesh("models/Box.glb");
int box = world.SpawnMeshEntity(meshId, new Transform { X = 2f });
```

## Destroying Entities

```csharp
world.Despawn(enemy);
```

`Despawn` removes the entity and all its attached components. It automatically handles cleanup:

- If the entity has a `MeshComponent`, the native renderer entity is removed via `NativeBridge.RemoveEntity()` — no manual cleanup needed
- If the entity has children (via `Hierarchy`), they are recursively despawned as well

## Checking Alive Status

```csharp
bool alive = world.IsAlive(entity);
```

## Multi-Entity Example

You can create many entities sharing the same mesh (instancing the same geometry):

```csharp
int meshId = NativeBridge.LoadMesh("models/Box.glb");

for (int i = 0; i < 10; i++)
{
    int e = world.SpawnMeshEntity(meshId, new Transform { X = i * 2f });
}
```

Each entity gets its own renderer draw slot but shares the GPU mesh data.

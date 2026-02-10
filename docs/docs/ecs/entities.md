---
sidebar_position: 2
---

# Entities

An entity is just an integer ID. It has no data of its own — all state is stored in [components](./components.md).

## Creating Entities

```csharp
int player = world.Spawn();
int enemy = world.Spawn();
```

Each call to `Spawn()` returns a unique integer ID.

## Destroying Entities

```csharp
world.Despawn(enemy);
```

`Despawn` removes the entity and all its attached components. If the entity has a `MeshComponent`, you should clean up the renderer entity first:

```csharp
var mc = world.GetComponent<MeshComponent>(entity);
if (mc != null && mc.RendererEntityId >= 0)
    NativeBridge.RemoveEntity(mc.RendererEntityId);

world.Despawn(entity);
```

## Multi-Entity Example

You can create many entities sharing the same mesh (instancing the same geometry):

```csharp
int meshId = NativeBridge.LoadMesh("models/Box.glb");

for (int i = 0; i < 10; i++)
{
    int e = world.Spawn();
    world.AddComponent(e, new Transform { X = i * 2f });
    world.AddComponent(e, new MeshComponent {
        MeshId = meshId,
        RendererEntityId = NativeBridge.CreateEntity(meshId)
    });
}
```

Each entity gets its own renderer draw slot but shares the GPU mesh data.

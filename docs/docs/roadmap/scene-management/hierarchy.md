# Parent-Child Hierarchy

:::tip Implemented
This feature is fully implemented.
:::

Entity relationships (sword attached to hand, wheel on car) via `Hierarchy` and `WorldTransform` components, with automatic transform inheritance and cascade despawn.

## Hierarchy Component

```csharp
world.AddComponent(child, new Hierarchy { Parent = parentEntity });
```

| Field    | Type  | Default | Description                                           |
| -------- | ----- | ------- | ----------------------------------------------------- |
| `Parent` | `int` | `-1`    | Entity ID of the parent, or `-1` for root (no parent) |

## WorldTransform Component

Stores the computed world-space matrix for entities in a hierarchy. Managed automatically by `HierarchyTransformSystem`.

```csharp
var wt = world.GetComponent<WorldTransform>(entity);
float[] worldMatrix = wt.Matrix;  // column-major 4x4
```

| Field    | Type        | Description                                   |
| -------- | ----------- | --------------------------------------------- |
| `Matrix` | `float[16]` | Column-major 4x4 world-space transform matrix |

## HierarchyTransformSystem

Runs each frame to compute world transforms:

1. For entities with a `Hierarchy` component pointing to a valid parent, it walks the parent chain and multiplies `parent.WorldTransform * child.LocalTransform`
2. A `WorldTransform` component is automatically added to child entities if not present
3. Root entities keep their local transform as-is

## Cascade Despawn

When a parent entity is despawned, all children (entities whose `Hierarchy.Parent` points to it) are recursively despawned as well, including native renderer cleanup.

## RenderSyncSystem Integration

`RenderSyncSystem` automatically uses `WorldTransform.Matrix` if available, falling back to `Transform.ToMatrix()` for root entities without hierarchy.

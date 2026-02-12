# Instanced Rendering

:::info Planned
This feature is not yet implemented.
:::

One draw call for many copies of the same mesh.

## Planned Scope

- Instance buffer with per-instance transforms
- Single `vkCmdDrawIndexed` with instance count
- Automatic batching of entities sharing the same mesh
- Significant draw call reduction for scenes with repeated geometry

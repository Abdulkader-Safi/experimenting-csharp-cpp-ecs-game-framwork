# Scene Save/Load

:::info Planned
This feature is not yet implemented.
:::

Serialize the ECS world to a file and restore it (save games).

## Planned Scope

- Serialize all entities and components to JSON or binary
- Deserialize and recreate the full ECS world state
- Handle mesh/resource references (reload from asset paths)
- Save/load API callable from C# systems

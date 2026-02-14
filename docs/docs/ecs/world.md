# World

The `World` is the container for all ECS state. It stores entities, their components, and the registered systems. Create one at startup.

```csharp
var world = new World();
```

## API

| Method                                                 | Description                                                                                              |
| ------------------------------------------------------ | -------------------------------------------------------------------------------------------------------- |
| `int Spawn()`                                          | Create a new entity, returns its ID                                                                      |
| `int Spawn(params object[] components)`                | Create a new entity with components attached, returns its ID                                             |
| `void Despawn(int entity)`                             | Destroy an entity, remove all components, auto-clean native renderer entity, and cascade-delete children |
| `bool IsAlive(int entity)`                             | Check if an entity ID is still alive                                                                     |
| `int SpawnMeshEntity(int meshId, Transform transform)` | Convenience: spawn entity with `Transform` + `MeshComponent` + native renderer registration in one call  |
| `void AddComponent<T>(int entity, T component)`        | Attach a component to an entity (generic)                                                                |
| `void AddComponent(int entity, object component)`      | Attach a component to an entity (non-generic, used by variadic Spawn)                                    |
| `T GetComponent<T>(int entity)`                        | Get a component (returns `null` if missing)                                                              |
| `bool HasComponent<T>(int entity)`                     | Check if an entity has a component                                                                       |
| `List<int> Query(params Type[] types)`                 | Find all entities with **all** given component types                                                     |
| `void AddSystem(Action<World> system)`                 | Register a system function                                                                               |
| `void RunSystems()`                                    | Execute all registered systems in order                                                                  |

## Properties

| Property    | Type    | Description                                                                                           |
| ----------- | ------- | ----------------------------------------------------------------------------------------------------- |
| `DeltaTime` | `float` | Time elapsed since last frame (seconds). Set each frame from `NativeBridge.renderer_get_delta_time()` |

## Example

```csharp
var world = new World();

// Spawn entities with components in one call
int player = world.Spawn(
    new Transform(),
    new Movable { Speed = 90f }
);

int enemy = world.Spawn(
    new Transform { Position = new Vec3(5f, 0f, 0f) }
);

// Query
List<int> movers = world.Query(typeof(Transform), typeof(Movable));

// Run all systems
world.RunSystems();

// Destroy
world.Despawn(enemy);
```

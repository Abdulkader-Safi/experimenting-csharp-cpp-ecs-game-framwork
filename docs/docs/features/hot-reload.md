# Hot Reload

`make dev` enables live code reloading for all files in `game_logic/`. Edit any `.cs` file, save, and the changes take effect immediately — no restart required.

## How It Works

The engine splits C# into three assemblies:

| Assembly        | Contents                                         | Reloadable? |
| --------------- | ------------------------------------------------ | ----------- |
| `Engine.dll`    | World, Components, NativeBridge, FreeCameraState | No          |
| `GameLogic.dll` | Game.cs, Systems.cs, GameConstants.cs            | Yes         |
| `ViewerDev.exe` | Viewer.cs + HotReload.cs                         | No          |

A `FileSystemWatcher` monitors `game_logic/` for `.cs` changes. On save:

1. All `.cs` files in `game_logic/` are auto-discovered and recompiled into a new `GameLogic.dll`
2. The new assembly is loaded via reflection
3. The world is **fully reset** — all entities are despawned (with native renderer cleanup), component stores and systems are cleared, and all 8 light slots are cleared
4. `Game.Setup(world)` is re-invoked from the new assembly, rebuilding the entire scene

This means **any** game logic change takes effect immediately:

- New or removed entities in `Game.cs`
- Changed light types or positions
- Updated constants in `GameConstants.cs`
- Modified system logic in `Systems.cs`
- Re-ordered system registration

## Usage

```bash
make dev
```

Then edit any file in `game_logic/` and save. The console will show:

```
[HotReload] Recompiling v1...
[HotReload] OK v1 - Scene re-initialized
```

## Limitations

- Only `game_logic/` files are hot-reloadable. Engine changes in `managed/` require a restart.
- Old assemblies leak memory (Mono cannot unload assemblies). Restart after many reloads.
- Static fields in `Systems.cs` and `GameConstants.cs` values reset on each reload.
- `make run` and `make app` are unaffected — they compile everything into a single `Viewer.exe` with no hot reload code.

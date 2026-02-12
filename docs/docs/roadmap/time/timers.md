# Timers

:::tip Implemented
This feature is fully implemented.
:::

Countdown and interval timers for cooldowns, spawning, and delays, implemented as a `Timer` component and `TimerSystem`.

## Timer Component

```csharp
world.AddComponent(entity, new Timer {
    Duration = 2.0f,    // seconds until timer fires
    Repeat = true,      // true = interval timer, false = one-shot
    Tag = "spawn"       // optional label for identification
});
```

| Field | Type | Default | Description |
|---|---|---|---|
| `Duration` | `float` | `1` | Time in seconds until the timer fires |
| `Elapsed` | `float` | `0` | Current elapsed time (managed by `TimerSystem`) |
| `Repeat` | `bool` | `false` | If `true`, resets and fires again each interval |
| `Finished` | `bool` | `false` | Set to `true` when elapsed reaches duration |
| `Tag` | `string` | `""` | Optional label to identify timer purpose |

## TimerSystem

The `TimerSystem` ticks all `Timer` components each frame using `world.DeltaTime`:

- **One-shot timers** (`Repeat = false`): `Finished` is set to `true` once and the timer stops
- **Interval timers** (`Repeat = true`): `Finished` is set to `true` each cycle, and `Elapsed` wraps by subtracting `Duration`

Check `timer.Finished` in your own systems to react to timer events.

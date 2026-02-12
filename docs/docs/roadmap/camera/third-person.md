# Third-Person Camera

:::tip Implemented
This feature is fully implemented.
:::

Orbit camera that follows the entity at a configurable distance, with scroll wheel zoom.

## How It Works

When `Camera.Mode = 0` (third-person, default):

1. **Orbit distance** is calculated from the offset vector length
2. **Scroll wheel** adjusts the orbit distance, clamped between `MinDistance` and `MaxDistance`
3. **Yaw and Pitch** define spherical coordinates around the entity
4. The camera eye position orbits around the entity's `Transform` position

## Camera Properties

| Field | Type | Default | Description |
|---|---|---|---|
| `Mode` | `int` | `0` | `0` = third-person (default), `1` = first-person |
| `MinDistance` | `float` | `1` | Minimum zoom distance |
| `MaxDistance` | `float` | `20` | Maximum zoom distance |
| `ZoomSpeed` | `float` | `2` | Scroll wheel zoom sensitivity |
| `OffsetX/Y/Z` | `float` | `0, 0, 3` | Offset vector; length = initial orbit distance |

## Controls

| Input | Action |
|---|---|
| Q / E | Orbit left / right (yaw) |
| R / F | Orbit up / down (pitch) |
| Mouse movement (when locked) | Free-look orbit |
| Scroll wheel | Zoom in / out |
| ESC | Toggle cursor lock |
| TAB | Switch to first-person mode |

# First-Person Camera

:::tip Implemented
This feature is fully implemented.
:::

Camera placed at the entity's eye position, looking in the yaw/pitch direction. Toggle with TAB.

## How It Works

When `Camera.Mode = 1` (first-person):

1. **Eye position** is the entity's `Transform` position plus `EyeHeight` on the Y axis
2. **Look direction** is computed from `Yaw` and `Pitch` using spherical coordinates
3. The view matrix is set via `NativeBridge.SetCamera(eye, eye + direction, up, fov)`

```csharp
// First-person mode sets:
// eye = (entity.X, entity.Y + EyeHeight, entity.Z)
// target = eye + (cos(pitch)*sin(yaw), sin(pitch), cos(pitch)*cos(yaw))
```

## Camera Properties

| Field | Type | Default | Description |
|---|---|---|---|
| `Mode` | `int` | `0` | `0` = third-person, `1` = first-person |
| `EyeHeight` | `float` | `0.8` | Vertical offset above entity position for eye placement |

## Switching Modes

Press **TAB** to toggle between first-person and third-person. The toggle is edge-detected (fires once per key press, not while held).

## Controls

Mouse look and keyboard orbit (Q/E/R/F) work the same as in third-person mode. Scroll wheel zoom is ignored in first-person mode.

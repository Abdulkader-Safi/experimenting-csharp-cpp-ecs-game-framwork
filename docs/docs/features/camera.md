---
sidebar_position: 3
---

# Camera

The engine provides a camera system with two modes: **third-person orbit** (default) and **first-person**. Toggle between them with TAB.

## Setup

Attach a `Camera` component to any entity with a `Transform`:

```csharp
world.AddComponent(player, new Camera {
    OffsetX = 0f, OffsetY = 0f, OffsetZ = 3f,
    Yaw = 0f, Pitch = 0f,
    Fov = 45f,
    LookSpeed = 90f,
    MouseSensitivity = 0.15f
});
```

The `CameraFollowSystem` handles all camera logic automatically.

## Camera Modes

### Third-Person (Mode 0, default)

The camera orbits around the entity at a configurable distance. Scroll wheel zooms in and out, clamped between `MinDistance` and `MaxDistance`.

### First-Person (Mode 1)

The camera is placed at the entity's position plus `EyeHeight` on the Y axis. It looks in the direction defined by `Yaw` and `Pitch`. Scroll wheel zoom is not used in this mode.

### Switching Modes

Press **TAB** to toggle between modes. The toggle is edge-detected (fires once per press).

## Controls

| Input | Action |
|---|---|
| Q / E | Orbit camera left / right (yaw) |
| R / F | Orbit camera up / down (pitch) |
| Mouse movement (when locked) | Free-look orbit |
| Scroll wheel | Zoom in / out (third-person only) |
| TAB | Toggle first-person / third-person |
| ESC | Toggle cursor lock on/off |

## How It Works

1. **Orbit distance** is calculated from the length of the offset vector (`OffsetX/Y/Z`)
2. **Yaw and Pitch** define the spherical coordinates around the entity
3. In third-person mode, the camera position is computed as: entity position + spherical offset
4. In first-person mode, the camera is at entity position + `EyeHeight`, looking along the yaw/pitch direction
5. Pitch is clamped to [-89, 89] degrees to prevent gimbal lock
6. The view matrix is set via `NativeBridge.SetCamera()`

## Mouse Look

When the cursor is locked (press ESC to toggle):
- Mouse movement updates `Yaw` and `Pitch` based on `MouseSensitivity`
- The cursor is hidden and captured for raw input
- Press ESC again to release the cursor

When the cursor is free, only keyboard orbit (Q/E/R/F) works.

## Default Camera

If no `Camera` component is attached to any entity, the renderer uses its built-in default:
- Eye position: `(0, 0, 3)`
- Look-at target: `(0, 0, 0)`
- Up vector: `(0, 1, 0)`
- FOV: 45 degrees

## Camera Properties

| Field | Type | Default | Description |
|---|---|---|---|
| `OffsetX/Y/Z` | `float` | `0, 0, 3` | Offset vector; length = orbit distance |
| `Yaw` | `float` | `0` | Horizontal orbit angle (degrees) |
| `Pitch` | `float` | `0` | Vertical orbit angle (degrees), clamped [-89, 89] |
| `Fov` | `float` | `45` | Vertical field of view (degrees) |
| `LookSpeed` | `float` | `90` | Degrees/second for keyboard orbit |
| `MouseSensitivity` | `float` | `0.15` | Degrees/pixel for mouse look |
| `Mode` | `int` | `0` | `0` = third-person orbit, `1` = first-person |
| `EyeHeight` | `float` | `0.8` | Eye height offset for first-person mode |
| `WasModeTogglePressed` | `bool` | `false` | Internal state for edge-detecting TAB |
| `MinDistance` | `float` | `1` | Minimum zoom distance (third-person) |
| `MaxDistance` | `float` | `20` | Maximum zoom distance (third-person) |
| `ZoomSpeed` | `float` | `2` | Scroll wheel zoom sensitivity |

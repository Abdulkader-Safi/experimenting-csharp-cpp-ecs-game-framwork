---
sidebar_position: 3
---

# Camera

The engine provides an orbit camera that follows an entity. It supports both keyboard and mouse controls.

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

## Controls

| Input | Action |
|---|---|
| Q / E | Orbit camera left / right (yaw) |
| R / F | Orbit camera up / down (pitch) |
| Mouse movement (when locked) | Free-look orbit |
| ESC | Toggle cursor lock on/off |

## How It Works

1. **Orbit distance** is calculated from the length of the offset vector (`OffsetX/Y/Z`)
2. **Yaw and Pitch** define the spherical coordinates around the entity
3. The camera position is computed as: entity position + spherical offset
4. Pitch is clamped to [-89, 89] degrees to prevent gimbal lock
5. The view matrix is set via `NativeBridge.SetCamera()`

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

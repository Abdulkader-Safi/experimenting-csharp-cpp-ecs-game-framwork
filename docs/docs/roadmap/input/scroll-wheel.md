---
sidebar_position: 4
---

# Scroll Wheel

:::tip Implemented
This feature is fully implemented.
:::

Scroll wheel input via a GLFW scroll callback, exposed to C# as an accumulator that can be read and reset each frame.

## API

```csharp
float scrollX, scrollY;
NativeBridge.GetScrollOffset(out scrollX, out scrollY);
NativeBridge.ResetScrollOffset();

// scrollY > 0 = scroll up, scrollY < 0 = scroll down
```

### NativeBridge Methods

| Method | Description |
|---|---|
| `void GetScrollOffset(out float x, out float y)` | Read accumulated scroll delta since last reset |
| `void ResetScrollOffset()` | Reset the scroll accumulator to zero |

## Implementation Details

- GLFW scroll callback accumulates offsets in the C++ renderer
- `GetScrollOffset` reads the accumulated value; `ResetScrollOffset` clears it
- Used by `CameraFollowSystem` for third-person camera zoom (scroll up = zoom in, scroll down = zoom out)
- Zoom distance is clamped between `Camera.MinDistance` and `Camera.MaxDistance`

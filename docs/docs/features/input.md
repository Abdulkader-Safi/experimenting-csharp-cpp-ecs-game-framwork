---
sidebar_position: 4
---

# Input

The engine provides keyboard and mouse input through GLFW, exposed to C# via `NativeBridge`.

## Keyboard Input

Poll individual keys each frame:

```csharp
if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W))
{
    // W is held down
}
```

### Available Key Constants

| Constant | Key |
|---|---|
| `GLFW_KEY_W/A/S/D` | WASD movement |
| `GLFW_KEY_Q/E` | Camera yaw |
| `GLFW_KEY_R/F` | Camera pitch |
| `GLFW_KEY_ESCAPE` | Toggle cursor lock |
| `GLFW_KEY_TAB` | Toggle camera mode (FP/TP) |
| `GLFW_KEY_UP/DOWN/LEFT/RIGHT` | Arrow keys |

For other keys, pass the [GLFW key code](https://www.glfw.org/docs/latest/group__keys.html) integer directly.

## Mouse Input

### Cursor Position

```csharp
double mx, my;
NativeBridge.GetCursorPos(out mx, out my);
```

Returns the cursor position in window coordinates.

### Cursor Lock

```csharp
NativeBridge.SetCursorLocked(true);   // capture + hide cursor
NativeBridge.SetCursorLocked(false);  // release cursor

bool locked = NativeBridge.IsCursorLocked();
```

When locked, the cursor is hidden and raw mouse input is enabled for camera look. The `CameraFollowSystem` uses ESC to toggle this automatically.

## Mouse Buttons

Poll individual mouse buttons each frame:

```csharp
if (NativeBridge.IsMouseButtonPressed(NativeBridge.GLFW_MOUSE_BUTTON_LEFT))
{
    // Left mouse button is held down
}
```

### Button Constants

| Constant | Value | Button |
|---|---|---|
| `GLFW_MOUSE_BUTTON_LEFT` | `0` | Left click |
| `GLFW_MOUSE_BUTTON_RIGHT` | `1` | Right click |
| `GLFW_MOUSE_BUTTON_MIDDLE` | `2` | Middle click (scroll wheel press) |

## Scroll Wheel

Read accumulated scroll delta and reset each frame:

```csharp
float scrollX, scrollY;
NativeBridge.GetScrollOffset(out scrollX, out scrollY);
NativeBridge.ResetScrollOffset();
```

| Method | Description |
|---|---|
| `GetScrollOffset(out float x, out float y)` | Read accumulated scroll delta since last reset |
| `ResetScrollOffset()` | Reset the scroll accumulator to zero |

The `CameraFollowSystem` uses scroll wheel for third-person camera zoom automatically.

## Input in Systems

The `InputMovementSystem` demonstrates how to use input in a system:

```csharp
public static void InputMovementSystem(World world)
{
    var entities = world.Query(typeof(Movable), typeof(Transform));
    foreach (int e in entities)
    {
        var mov = world.GetComponent<Movable>(e);
        var tr = world.GetComponent<Transform>(e);
        float moveStep = mov.Speed * world.DeltaTime;

        if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_A))
            tr.RotY -= moveStep;
        if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_D))
            tr.RotY += moveStep;
    }
}
```

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

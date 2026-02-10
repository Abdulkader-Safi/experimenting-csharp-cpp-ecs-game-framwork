---
sidebar_position: 3
---

# Mouse Buttons

:::tip Implemented
This feature is fully implemented.
:::

Left, right, and middle mouse button polling via GLFW, exposed to C# through `NativeBridge`.

## API

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

### NativeBridge Methods

| Method | Description |
|---|---|
| `bool IsMouseButtonPressed(int button)` | Returns `true` if the button is currently held down |

## Implementation Details

- Polls GLFW mouse button state each frame via `renderer_is_mouse_button_pressed()`
- Returns press state (held down), not click events
- For other buttons, pass the [GLFW mouse button code](https://www.glfw.org/docs/latest/group__buttons.html) integer directly

---
sidebar_position: 1
---

# NativeBridge Reference

`NativeBridge` (`managed/ecs/NativeBridge.cs`) provides all P/Invoke bindings to the C++ Vulkan renderer.

## Renderer Lifecycle

```csharp
NativeBridge.renderer_init(800, 600, "Window Title");  // create window + Vulkan
NativeBridge.renderer_cleanup();                        // destroy everything
```

## Main Loop

```csharp
NativeBridge.renderer_should_close();  // true when user closes window
NativeBridge.renderer_poll_events();   // process window/input events
NativeBridge.renderer_render_frame();  // draw all active entities
```

## Mesh & Entity Management

```csharp
int meshId = NativeBridge.LoadMesh("path/to/model.glb");
int entityId = NativeBridge.CreateEntity(meshId);
NativeBridge.SetEntityTransform(entityId, float[16] matrix);
NativeBridge.RemoveEntity(entityId);
```

| Method | Returns | Description |
|---|---|---|
| `LoadMesh(path)` | `int` | Load a glTF file, returns mesh ID |
| `CreateEntity(meshId)` | `int` | Create a draw slot for a mesh, returns entity ID |
| `SetEntityTransform(id, matrix)` | `void` | Set 4x4 model matrix (column-major `float[16]`) |
| `RemoveEntity(id)` | `void` | Destroy a draw slot |

## Camera

```csharp
NativeBridge.SetCamera(
    eyeX, eyeY, eyeZ,          // camera position
    targetX, targetY, targetZ,  // look-at point
    upX, upY, upZ,              // up vector
    fovDegrees                  // vertical FOV
);
```

Sets the view and projection matrices. If never called, defaults to eye `(0,0,3)`, target `(0,0,0)`, up `(0,1,0)`, FOV `45`. Typically called by `CameraFollowSystem` rather than directly.

## Input

```csharp
bool pressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W);
```

See [Input](../features/input.md) for the full list of key constants.

## Cursor

```csharp
NativeBridge.SetCursorLocked(true);   // capture cursor
NativeBridge.SetCursorLocked(false);  // release cursor
bool locked = NativeBridge.IsCursorLocked();

double mx, my;
NativeBridge.GetCursorPos(out mx, out my);
```

## Time

```csharp
NativeBridge.renderer_update_time();            // sample glfwGetTime, compute delta
float dt = NativeBridge.renderer_get_delta_time();   // seconds since last update
float total = NativeBridge.renderer_get_total_time(); // seconds since init
```

## Lighting

```csharp
NativeBridge.SetLight(index, type,
    posX, posY, posZ,
    dirX, dirY, dirZ,
    r, g, b, intensity,
    radius, innerCone, outerCone);

NativeBridge.ClearLight(index);     // disable a light slot
NativeBridge.SetAmbient(intensity); // set ambient light level
```

| Parameter | Description |
|---|---|
| `index` | Light slot (0–7) |
| `type` | 0 = directional, 1 = point, 2 = spot |
| `pos` | World position (point and spot lights) |
| `dir` | Direction vector (directional and spot) |
| `r, g, b` | Light color |
| `intensity` | Brightness multiplier |
| `radius` | Attenuation distance (point and spot) |
| `innerCone/outerCone` | Cosine of cone angles (spot only) |

:::note
The `innerCone` and `outerCone` parameters expect **cosine values**, not degrees. The `LightSyncSystem` handles this conversion automatically from the `Light` component's degree fields.
:::

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

| Method                           | Returns | Description                                      |
| -------------------------------- | ------- | ------------------------------------------------ |
| `LoadMesh(path)`                 | `int`   | Load a glTF file, returns mesh ID                |
| `CreateEntity(meshId)`           | `int`   | Create a draw slot for a mesh, returns entity ID |
| `SetEntityTransform(id, matrix)` | `void`  | Set 4x4 model matrix (column-major `float[16]`)  |
| `RemoveEntity(id)`               | `void`  | Destroy a draw slot                              |

## Procedural Primitives

Generate meshes for common 3D shapes without external model files. Each method returns a mesh ID usable with `CreateEntity()`.

```csharp
int box  = NativeBridge.CreateBoxMesh(2f, 1f, 3f, 0.8f, 0.2f, 0.2f);
int sph  = NativeBridge.CreateSphereMesh(0.5f, 32, 16, 0.2f, 0.8f, 0.2f);
int pln  = NativeBridge.CreatePlaneMesh(10f, 10f, 0.3f, 0.8f, 0.3f);
int cyl  = NativeBridge.CreateCylinderMesh(0.4f, 1f, 32, 0.2f, 0.2f, 0.8f);
int cap  = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f, 32, 16, 0.9f, 0.8f, 0.2f);
```

| Method               | Parameters                                 | Description                              |
| -------------------- | ------------------------------------------ | ---------------------------------------- |
| `CreateBoxMesh`      | `w, h, l, r, g, b`                         | Axis-aligned box (width, height, length) |
| `CreateSphereMesh`   | `radius, segments, rings, r, g, b`         | UV sphere                                |
| `CreatePlaneMesh`    | `w, h, r, g, b`                            | Flat quad on XZ plane                    |
| `CreateCylinderMesh` | `radius, height, segments, r, g, b`        | Capped cylinder along Y                  |
| `CreateCapsuleMesh`  | `radius, height, segments, rings, r, g, b` | Cylinder with hemisphere caps            |

All shapes have convenience overloads that use default grey color (`0.7`) and default tessellation (32 segments, 16 rings):

```csharp
int box = NativeBridge.CreateBoxMesh(1f, 1f, 1f);
int sph = NativeBridge.CreateSphereMesh(0.5f);
int pln = NativeBridge.CreatePlaneMesh(10f, 10f);
int cyl = NativeBridge.CreateCylinderMesh(0.4f, 1f);
int cap = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f);
```

See [Primitive Shapes](../features/primitive-shapes.md) for full parameter tables and ECS usage examples.

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

| Parameter             | Description                             |
| --------------------- | --------------------------------------- |
| `index`               | Light slot (0-7)                        |
| `type`                | 0 = directional, 1 = point, 2 = spot    |
| `pos`                 | World position (point and spot lights)  |
| `dir`                 | Direction vector (directional and spot) |
| `r, g, b`             | Light color                             |
| `intensity`           | Brightness multiplier                   |
| `radius`              | Attenuation distance (point and spot)   |
| `innerCone/outerCone` | Cosine of cone angles (spot only)       |

:::note
The `innerCone` and `outerCone` parameters expect **cosine values**, not degrees. The `LightSyncSystem` handles this conversion automatically from the `Light` component's degree fields.
:::

## Debug Overlay

```csharp
NativeBridge.SetDebugOverlay(true);   // show overlay
NativeBridge.SetDebugOverlay(false);  // hide overlay
int count = NativeBridge.GetEntityCount();  // active entities in renderer
```

| Method                  | Returns | Description                                              |
| ----------------------- | ------- | -------------------------------------------------------- |
| `SetDebugOverlay(bool)` | `void`  | Enable/disable the debug overlay (FPS, DT, entity count) |
| `GetEntityCount()`      | `int`   | Number of active entities in the C++ renderer            |

The overlay is managed by `DebugOverlaySystem` which toggles it via the F3 key. See [Debug Overlay](../features/debug-overlay.md) for details.

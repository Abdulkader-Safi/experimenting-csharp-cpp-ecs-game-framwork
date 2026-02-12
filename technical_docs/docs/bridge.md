# P/Invoke Bridge

## Overview

`bridge.cpp` is the interface layer between C# (Mono) and C++. It contains:

- A global `static VulkanRenderer g_renderer;` instance
- `extern "C"` functions that delegate to `g_renderer` methods
- Try/catch wrappers for functions that may throw

C# calls these functions via `[DllImport("renderer")]` in `NativeBridge.cs`.

## Bridge Functions by Category

### Lifecycle

| C Bridge                            | C++ Method      | Notes     |
| ----------------------------------- | --------------- | --------- |
| `renderer_init(w, h, title)` → bool | `init()`        | try/catch |
| `renderer_cleanup()`                | `cleanup()`     | try/catch |
| `renderer_should_close()` → bool    | `shouldClose()` |           |
| `renderer_poll_events()`            | `pollEvents()`  |           |
| `renderer_render_frame()`           | `renderFrame()` | try/catch |

### Input

| C Bridge                                      | C++ Method               | Notes       |
| --------------------------------------------- | ------------------------ | ----------- |
| `renderer_is_key_pressed(key)` → int          | `isKeyPressed()`         | returns 0/1 |
| `renderer_get_cursor_pos(x*, y*)`             | `getCursorPos()`         | out params  |
| `renderer_set_cursor_locked(locked)`          | `setCursorLocked()`      | int→bool    |
| `renderer_is_cursor_locked()` → int           | `isCursorLocked()`       | bool→int    |
| `renderer_is_mouse_button_pressed(btn)` → int | `isMouseButtonPressed()` | returns 0/1 |
| `renderer_get_scroll_offset(x*, y*)`          | `getScrollOffset()`      | out params  |
| `renderer_reset_scroll_offset()`              | `resetScrollOffset()`    |             |

### Mesh

| C Bridge                                  | C++ Method             | Notes                        |
| ----------------------------------------- | ---------------------- | ---------------------------- |
| `renderer_load_mesh(path)` → int          | `loadMesh()`           | try/catch, returns mesh ID   |
| `renderer_create_entity(mesh_id)` → int   | `createEntity()`       | try/catch, returns entity ID |
| `renderer_set_entity_transform(id, mat4)` | `setEntityTransform()` | try/catch                    |
| `renderer_remove_entity(id)`              | `removeEntity()`       | try/catch                    |
| `renderer_load_model(path)` → bool        | `loadModel()`          | legacy, try/catch            |
| `renderer_set_rotation(rx, ry, rz)`       | `setRotation()`        | legacy                       |

### Procedural Primitives

| C Bridge                                                   | C++ Method             | Notes     |
| ---------------------------------------------------------- | ---------------------- | --------- |
| `renderer_create_box_mesh(w,h,l,r,g,b)` → int              | `createBoxMesh()`      | try/catch |
| `renderer_create_sphere_mesh(rad,seg,ring,r,g,b)` → int    | `createSphereMesh()`   | try/catch |
| `renderer_create_plane_mesh(w,h,r,g,b)` → int              | `createPlaneMesh()`    | try/catch |
| `renderer_create_cylinder_mesh(rad,h,seg,r,g,b)` → int     | `createCylinderMesh()` | try/catch |
| `renderer_create_capsule_mesh(rad,h,seg,ring,r,g,b)` → int | `createCapsuleMesh()`  | try/catch |

### Camera

| C Bridge                                             | C++ Method    |
| ---------------------------------------------------- | ------------- |
| `renderer_set_camera(eyeXYZ, targetXYZ, upXYZ, fov)` | `setCamera()` |

### Lighting

| C Bridge                                                                   | C++ Method              |
| -------------------------------------------------------------------------- | ----------------------- |
| `renderer_set_light(idx, type, pos, dir, color, intensity, radius, cones)` | `setLight()`            |
| `renderer_clear_light(idx)`                                                | `clearLight()`          |
| `renderer_set_ambient(intensity)`                                          | `setAmbientIntensity()` |

### Time

| C Bridge                            | C++ Method       |
| ----------------------------------- | ---------------- |
| `renderer_update_time()`            | `updateTime()`   |
| `renderer_get_delta_time()` → float | `getDeltaTime()` |
| `renderer_get_total_time()` → float | `getTotalTime()` |

### Debug

| C Bridge                              | C++ Method               |
| ------------------------------------- | ------------------------ | -------- |
| `renderer_set_debug_overlay(enabled)` | `setDebugOverlay()`      | int→bool |
| `renderer_get_entity_count()` → int   | `getActiveEntityCount()` |

## Type Mapping (C# → C++)

| C# Type            | C Bridge       | C++ Type                       |
| ------------------ | -------------- | ------------------------------ |
| `int`              | `int`          | `int`                          |
| `float`            | `float`        | `float`                        |
| `bool` (return)    | `bool`         | `bool`                         |
| `string`           | `const char*`  | `const char*`                  |
| `float[]`          | `const float*` | `const float*`                 |
| `out double`       | `double*`      | `double&`                      |
| `out float`        | `float*`       | `float&`                       |
| `int` (bool param) | `int`          | converted to `bool` via `!= 0` |

## NativeBridge.cs Convenience Wrappers

The C# side provides both raw `[DllImport]` declarations and convenience wrappers:

- `IsKeyPressed(key)` → converts `int` return to `bool`
- `SetCursorLocked(bool)` → converts `bool` to `int` parameter
- Overloaded primitives with default color (0.7, 0.7, 0.7) and segment counts (32 segments, 16 rings)
- GLFW key/mouse button constants defined as `const int`

:::tip Where to Edit
**Adding a new bridge function** (3-file checklist):

1. **`native/renderer.h`** — Declare the method on `VulkanRenderer` (public section)
2. **`native/renderer.cpp`** — Implement the method
3. **`native/bridge.cpp`** — Add `extern "C"` wrapper calling `g_renderer.method()`. Use try/catch if it can throw.
4. **`managed/ecs/NativeBridge.cs`** — Add `[DllImport(LIB)]` declaration + optional convenience wrapper
   :::

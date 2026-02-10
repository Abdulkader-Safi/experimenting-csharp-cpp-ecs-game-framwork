---
sidebar_position: 5
---

# Delta Time

The engine provides frame-independent timing through native C++ (`glfwGetTime`), exposed to C# via P/Invoke.

## Usage

Update the timer each frame and read delta time:

```csharp
// In your main loop:
NativeBridge.renderer_update_time();
world.DeltaTime = NativeBridge.renderer_get_delta_time();
```

Then use `world.DeltaTime` in your systems for frame-independent movement:

```csharp
float moveStep = speed * world.DeltaTime;
```

## API

| Method | Returns | Description |
|---|---|---|
| `renderer_update_time()` | `void` | Samples `glfwGetTime()` and computes delta |
| `renderer_get_delta_time()` | `float` | Seconds elapsed since last `update_time()` call |
| `renderer_get_total_time()` | `float` | Total seconds since renderer init |

## Why Native Timing?

Delta time is computed in C++ using `glfwGetTime()` rather than C#'s `Stopwatch` because:
- GLFW's timer is tied to the same clock as the event loop
- Avoids overhead of crossing the P/Invoke boundary for high-frequency timing
- Consistent with how other native systems (input, rendering) measure time

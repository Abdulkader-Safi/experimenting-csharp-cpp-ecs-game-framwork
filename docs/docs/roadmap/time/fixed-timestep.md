# Fixed Timestep

:::tip Implemented
This feature is implemented in `PhysicsWorld`.
:::

The physics simulation uses a fixed 1/60s timestep with an accumulator pattern, ensuring consistent behavior regardless of rendering frame rate.

## How It Works

- Each frame, `DeltaTime` is added to an accumulator in `PhysicsWorld`
- While the accumulator has enough time (>= 1/60s), Jolt Physics is stepped at the fixed rate
- Maximum 4 steps per frame to prevent spiral-of-death on frame drops
- If the accumulator exceeds the max budget, it resets to zero

This means physics always runs at 60 Hz internally, even if the renderer runs at 30 FPS or 144 FPS.

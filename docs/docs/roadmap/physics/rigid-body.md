# Rigid Body Dynamics

:::tip Implemented
This feature is implemented using Jolt Physics via joltc.
:::

Rigid body dynamics are provided by Jolt Physics through the `Rigidbody` component.

## What's Implemented

- **Motion types** — Static (immovable), Kinematic (scripted), Dynamic (physics-driven)
- **Gravity** — Configurable per-body gravity factor (default world gravity: 0, -9.81, 0)
- **Friction** — Surface friction coefficient per body
- **Restitution** — Bounciness per body (0 = no bounce, 1 = perfectly elastic)
- **Linear damping** — Velocity decay over time
- **Angular damping** — Angular velocity decay over time
- **Fixed timestep** — 1/60s accumulator pattern for stable simulation
- **Auto body creation** — `PhysicsSystem` automatically creates Jolt bodies for entities with `Rigidbody` + `Collider` + `Transform`
- **Transform sync** — Position and rotation are read back from Jolt each frame for dynamic bodies

See the [Physics feature page](../../features/physics.md) for usage examples.

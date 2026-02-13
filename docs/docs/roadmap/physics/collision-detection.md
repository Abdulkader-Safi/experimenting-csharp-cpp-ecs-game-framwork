# Collision Detection

:::tip Implemented
This feature is implemented using Jolt Physics via joltc.
:::

Collision detection is handled by Jolt Physics through the `Collider` component and `PhysicsSystem`.

## What's Implemented

- **Box collider** — axis-aligned box with configurable half-extents
- **Sphere collider** — sphere with configurable radius
- **Capsule collider** — capsule with configurable half-height and radius
- **Cylinder collider** — cylinder with configurable half-height and radius
- **Plane collider** — infinite plane with configurable normal, distance, and half-extent
- **Broad-phase optimization** — Jolt's built-in broadphase with two layers (moving/non-moving)
- **Continuous collision** — handled internally by Jolt Physics

See the [Physics feature page](../../features/physics.md) for usage examples.

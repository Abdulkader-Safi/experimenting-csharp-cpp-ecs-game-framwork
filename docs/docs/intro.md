---
sidebar_position: 1
slug: /intro
---

# Safi Game Engine

A **Bevy-inspired Entity Component System** where C# owns the game logic (components, systems, queries) and C++ is the rendering backend (Vulkan, multi-entity draw calls).

## Architecture

```
C# (Mono)                          C++ (Vulkan)
┌─────────────────────┐            ┌──────────────────────┐
│  World              │            │  VulkanRenderer      │
│   ├─ Entities       │            │   ├─ Meshes (GPU)    │
│   ├─ Components     │  P/Invoke  │   ├─ Entities        │
│   ├─ Systems ───────┼───────────>│   ├─ Lights (UBO)    │
│   └─ Queries        │            │   └─ Push Constants  │
│                     │            │      (per-entity     │
│                     │            │       model matrix)  │
└─────────────────────┘            └──────────────────────┘
```

The C# `World` tracks entities and their components. Systems run each frame, querying for entities with specific component combinations. The `RenderSyncSystem` and `LightSyncSystem` push data to the C++ renderer via `NativeBridge`.

## What's Implemented

| Feature                                                       | Status |
| ------------------------------------------------------------- | ------ |
| ECS (World, Entities, Components, Systems, Queries)           | Done   |
| 3D Mesh loading (glTF)                                        | Done   |
| Multi-entity rendering (push constants)                       | Done   |
| Transform component (position, rotation, scale, matrix)       | Done   |
| Orbit camera (keyboard + mouse look, ESC toggle)              | Done   |
| Keyboard input (polling)                                      | Done   |
| Mouse input (position, cursor lock)                           | Done   |
| Depth testing / back-face culling                             | Done   |
| Multiple dynamic lights (directional/point/spot, max 8)       | Done   |
| Delta time (native C++ via glfwGetTime)                       | Done   |
| macOS .app bundle                                             | Done   |
| Mouse buttons + scroll wheel                                  | Done   |
| Timers (countdown/interval)                                   | Done   |
| Runtime spawn/despawn with cleanup                            | Done   |
| Parent-child entity hierarchy                                 | Done   |
| First-person / third-person camera                            | Done   |
| Procedural primitives (box, sphere, plane, cylinder, capsule) | Done   |
| Free camera (debug fly camera)                                | Done   |

## Next Steps

- [Getting Started](./getting-started/prerequisites.md) — install dependencies and build
- [Quick Start](./getting-started/quick-start.md) — build your first scene in 5 minutes
- [ECS Guide](./ecs/world.md) — learn the entity component system
- [Roadmap](./roadmap/rendering/textures.md) — planned features

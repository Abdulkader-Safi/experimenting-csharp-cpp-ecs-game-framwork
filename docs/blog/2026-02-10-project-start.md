---
slug: project-start
title: "Building a Game Engine from Scratch: Why C# + C++?"
authors: [abdulkader]
tags: [announcement, engine, architecture]
---

This is the first post on the devlog for the C++/C# Game Engine — a Bevy-inspired Entity Component System where C# drives the game logic and C++ handles Vulkan rendering.

<!-- truncate -->

## Why Build a Game Engine?

The best way to truly understand how games work under the hood is to build the engine yourself. Not a Unity plugin, not a Godot module — the actual renderer, the ECS, the input pipeline, all of it. That was the goal from day one: learn by building.

## Why C# and C++?

Most engines pick one language. I wanted both:

- **C#** for game logic — fast iteration, type safety, garbage collection, and a clean syntax for defining components and systems. Mono makes it easy to run on macOS without pulling in the full .NET runtime.
- **C++** for rendering — direct Vulkan access, zero-overhead abstractions, and full control over GPU resources. No managed runtime sitting between you and the driver.

The two talk through **P/Invoke** — C# calls into a C++ shared library (`.dylib`) for everything render-related. It's the same pattern Unity uses internally, just stripped down to the essentials.

## What's Working So Far

The engine already has a solid foundation:

- **Entity Component System** — a `World` with entities, typed components, queries, and ordered system execution
- **Vulkan renderer** — glTF mesh loading, multi-entity draw calls with push constants, depth testing, back-face culling
- **Dynamic lighting** — up to 8 lights (directional, point, spot) with Blinn-Phong shading
- **Orbit camera** — keyboard and mouse look with ESC toggle for cursor lock
- **Input system** — keyboard polling and mouse position via GLFW
- **Delta time** — native timing through `glfwGetTime()` for frame-independent movement
- **macOS .app bundle** — `make app` produces a standalone application

## What's Next

There's a long [roadmap](/docs/roadmap/rendering/textures) ahead — textures, PBR materials, shadows, skeletal animation, physics, audio, and eventually cross-platform builds. The plan is to keep the engine minimal and educational, adding features one at a time with clear documentation for each.

If you want to try it out, head over to the [Getting Started](/docs/getting-started/prerequisites) guide.

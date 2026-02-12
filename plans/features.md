## What You Already Have

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
| Spawn/despawn with native cleanup                             | Done   |
| Parent-child entity hierarchy                                 | Done   |
| First-person / third-person camera                            | Done   |
| Procedural primitives (box, sphere, plane, cylinder, capsule) | Done   |
| Free camera (debug fly camera, press 0/1 to toggle)           | Done   |
| GameConstants (tunable config values)                         | Done   |
| UI rendering pipeline (2D quads, alpha blending, no depth)    | Done   |
| Text rendering (stb_truetype font atlas, R8 texture)          | Done   |
| Debug overlay (FPS, delta time, entity count, F3 toggle)      | Done   |
| Textures & UV mapping (glTF base color texture, stb_image)    | Done   |

--https://bevy.org/-

## What You Need

### 1. Rendering

- [x] **Textures & UV mapping** — Load textures from glTF, sample in fragment shader, add UV coords to vertex format
- [ ] **PBR materials** — Metallic/roughness workflow (base color, metallic, roughness, emissive maps)
- [x] **Procedural primitives** — Box, sphere, plane, cylinder, capsule generated without external files
- [x] **Multiple lights** — Point, directional, spot lights as ECS entities with position/color/intensity
- [ ] **Shadows** — Shadow mapping (at minimum one directional light)
- [ ] **Transparency / alpha blending** — Separate pass for transparent objects, sorted back-to-front
- [ ] **Skybox / environment** — Cubemap skybox for background and ambient reflections
- [ ] **MSAA** — Multi-sample anti-aliasing
- [ ] **Custom shaders / render graph** — Swap or extend shaders and render passes at runtime
- [ ] **Instanced rendering** — One draw call for many copies of the same mesh

### 2. Animation

- [ ] **Skeletal animation** — Bone hierarchy, skinning weights, joint transforms from glTF skins
- [ ] **Animation playback** — Play/pause/loop animation clips from glTF
- [ ] **Animation blending** — Crossfade between clips (idle → walk → run)

### 3. Physics & Collision

- [ ] **Collision detection** — AABB, sphere, or mesh-based collision checks
- [ ] **Rigid body dynamics** — Gravity, velocity, mass, forces, impulses
- [ ] **Raycasting** — Cast rays to detect what the player is looking at or shooting at
- [ ] **Character controller** — Grounded check, slope handling, step-up
- [ ] **Trigger volumes** — Detect entity enter/exit (doors, damage zones, pickups)

### 4. Scene Management

- [ ] **Scene save/load** — Serialize ECS world to file and restore (save games)
- [x] **Parent-child hierarchy** — Entity relationships (sword attached to hand, wheel on car)
- [ ] **Prefabs / templates** — Reusable entity blueprints (spawn enemy from template)
- [ ] **Level loading** — Load/switch between different scenes

### 5. Audio

- [ ] **Audio playback** — Load and play WAV/OGG files
- [ ] **Spatial audio** — 3D positioned sounds that attenuate with distance
- [ ] **Background music** — Looping track with volume control
- [ ] **Sound triggers** — Play sounds on game events (collision, pickup, footstep)

### 6. Input System

- [ ] **Gamepad support** — GLFW joystick/gamepad input
- [ ] **Input action mapping** — Map physical keys to logical actions ("Jump" → Space / A-button)
- [x] **Mouse buttons** — Left/right click detection (shoot, interact)
- [x] **Scroll wheel** — Camera zoom or weapon swap

### 7. UI / HUD

- [x] **Text rendering** — Font atlas via stb_truetype, screen-space text with configurable color
- [ ] **HUD elements** — Health bars, icons, crosshair overlaid on 3D scene
- [ ] **Menu system** — Start screen, pause menu, settings with navigation
- [x] **Debug overlay** — FPS counter, delta time, entity count (F3 toggle)

### 8. Camera System

- [x] **First-person camera** — Camera at entity eye position, no orbit
- [x] **Third-person camera** — Follow behind entity with wall collision avoidance
- [ ] **Camera shake** — Screen shake on impacts/explosions
- [ ] **Smooth interpolation** — Lerp position/rotation for smooth transitions

### 9. AI / Gameplay

- [ ] **Pathfinding** — A\* or navmesh for NPC navigation
- [ ] **State machine** — Idle/patrol/chase/attack states for enemies
- [x] **Spawn system** — Create entities at runtime (enemies, projectiles, particles)
- [ ] **Health / damage system** — Health component, damage events, death/despawn
- [ ] **Inventory / pickup** — Collect items, store and manage inventory

### 10. Asset Management

- [ ] **Asset manager** — Central registry to avoid double-loading meshes/textures/sounds
- [ ] **Hot reloading** — Detect file changes and reload assets at runtime
- [ ] **Async loading** — Load assets without blocking the main loop

### 11. Time & Delta Time

- [x] **Delta time** — Frame-independent movement via native `glfwGetTime()`, exposed to C# through FFI
- [ ] **Fixed timestep** — Consistent physics updates regardless of framerate
- [x] **Timers** — Countdown/interval timers for cooldowns, spawning, delays

### 12. Entity Lifecycle & Events

- [ ] **Event system** — Fire and handle events (OnCollision, OnDeath, OnPickup)
- [ ] **Component callbacks** — Run code when components are added/removed
- [ ] **Entity tags / layers** — Categorize entities (Player, Enemy, Projectile) for filtered queries

### 13. Cross-Platform (Bevy ships with this)

- [ ] **Windows / Linux builds** — Currently macOS only
- [ ] **Web export** — Emscripten + WebGPU/WebGL

---

## Suggested Priority Order

**Phase 1 — Core engine (make things move properly):**

1. ~~Delta time~~ ✓
2. ~~Parent-child entity hierarchy~~ ✓
3. ~~Mouse buttons + scroll wheel~~ ✓
4. ~~First-person / third-person camera~~ ✓
5. ~~Spawn/despawn at runtime~~ ✓
6. ~~Timers~~ ✓

**Phase 2 — Make it look good:**

7. ~~Textures & UV mapping~~ ✓
8. PBR materials
9. Multiple lights
10. Shadows
11. Skybox
12. MSAA

**Phase 3 — Make it a game:**

13. Collision detection (AABB)
14. Rigid body physics (gravity, velocity)
15. Raycasting
16. Health/damage system
17. Audio playback + spatial audio
18. Event system

**Phase 4 — Polish:**

19. Text rendering / HUD
20. Skeletal animation playback
21. Animation blending
22. Menu system
23. Input action mapping
24. Gamepad support

**Phase 5 — Advanced:**

25. Pathfinding / AI state machines
26. Scene save/load
27. Instanced rendering
28. Hot reloading
29. Cross-platform builds

# Debug Overlay

The engine includes a debug overlay that displays real-time performance information and physics collider wireframes rendered directly on the GPU.

## What It Shows

When enabled (press **F3**), the overlay provides two types of debug visualization:

### Text HUD

A semi-transparent dark panel at the top-left corner with three lines of green monospace text:

- **FPS** — Smoothed frames per second (exponential moving average: `0.95 * old + 0.05 * new`)
- **DT** — Delta time in milliseconds
- **Entities** — Number of active entities in the renderer

### Collider Wireframes

Wireframe outlines are drawn around all physics colliders, making it easy to verify collider placement, size, and alignment with visual meshes. Each collider's wireframe color is configurable via the `DebugColor` field (defaults to green). Supported shapes:

| Collider Shape | Wireframe Visualization                       |
| -------------- | --------------------------------------------- |
| Box            | Wireframe cube matching half-extents          |
| Sphere         | Wireframe sphere (16 segments, 8 rings)       |
| Capsule        | Wireframe capsule matching radius and height  |
| Cylinder       | Wireframe cylinder matching radius and height |
| Plane          | Skipped (too large to render meaningfully)    |

Wireframes follow the entity's transform, so they move and rotate with dynamic physics bodies.

```csharp
// Custom wireframe color per collider
new Collider { ShapeType = Collider.Box, DebugColor = Color.Red }
new Collider { ShapeType = Collider.Sphere, DebugColor = new Color("#ff6600") }
new Collider { ShapeType = Collider.Capsule, DebugColor = new Color(0.5f, 0f, 1f) }
```

## Toggle

Press **F3** to toggle the overlay on or off. The overlay is **enabled by default** when `GameConstants.Debug` is `true`.

```csharp
// DebugOverlaySystem handles the F3 toggle automatically.
// You can also control it directly:
NativeBridge.SetDebugOverlay(true);   // show
NativeBridge.SetDebugOverlay(false);  // hide
```

## How It Works

### Wireframe Pipeline

Collider wireframes use a dedicated Vulkan graphics pipeline created alongside the 3D scene pipeline:

- **Polygon mode** — `VK_POLYGON_MODE_LINE` (wireframe rendering, requires `fillModeNonSolid` device feature)
- **No backface culling** — wireframes are visible from all angles
- **Depth test** — enabled with `LESS_OR_EQUAL` compare (draws at same depth as solid geometry)
- **No depth write** — wireframes don't occlude other objects
- **Same shaders** — reuses `shader.vert` and `shader.frag` from the 3D pipeline
- **Same vertex/index buffers** — shares the combined geometry buffer

Debug entities are stored in a separate list (`debugEntities_`) from regular entities, rendered only when the debug overlay is enabled.

### UI Rendering Pipeline

The text HUD uses a dedicated Vulkan graphics pipeline separate from the 3D scene pipeline:

- **No depth testing** — UI always draws on top
- **Alpha blending** — `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` for transparency
- **No backface culling** — 2D quads don't need it
- **Push constant** — `vec2 screenSize` converts pixel coordinates to NDC

### Font Atlas

Text is rendered using a font atlas baked at init time with `stb_truetype`:

1. A TTF font file (`assets/fonts/RobotoMono-Regular.ttf`) is loaded
2. ASCII characters 32-126 are rasterized into a 512x512 `R8_UNORM` texture
3. Each character's glyph metrics (UVs, offsets, advance) are stored for quad generation
4. If the font file is missing, a 1x1 white placeholder is used and text rendering is skipped

### Shaders

| Shader    | Purpose                                                                 |
| --------- | ----------------------------------------------------------------------- |
| `ui.vert` | Converts pixel-space `vec2 pos` to NDC using `screenSize` push constant |
| `ui.frag` | Samples R8 font atlas, multiplies by vertex color and alpha             |

### Per-Frame Flow

1. `buildDebugOverlayGeometry()` generates UI vertices (background quad + text quads)
2. Vertices are uploaded to a host-visible, persistently-mapped buffer
3. `recordCommandBuffer()` renders: 3D entities → debug wireframes → UI text
4. `DebugColliderRenderSystem` creates/syncs wireframe entities matching each collider

### Zero Overhead When Disabled

When the overlay is off, no wireframe geometry is created, no UI geometry is built, and no debug draw calls are recorded. Toggling off clears all debug renderer entities.

## Systems

### DebugOverlaySystem

Runs each frame and handles:

- Edge-detected F3 key press to toggle `GameConstants.Debug`
- Calling `NativeBridge.SetDebugOverlay()` to sync the state to C++

### DebugColliderRenderSystem

Runs each frame after `DebugOverlaySystem` and handles:

- When debug turns **on**: creates wireframe meshes (colored per `Collider.DebugColor`) matching each collider shape and registers them as debug renderer entities
- Each frame while debug is **on**: syncs debug entity transforms to match their collider's position
- When debug turns **off**: calls `ClearDebugEntities()` and clears tracking state
- Automatically cleans up debug entities for despawned ECS entities

The system maintains a `Dictionary<int, int>` mapping ECS entity IDs to debug renderer entity IDs.

## System Chain

```
InputMovement -> Timer -> Physics -> FreeCamera -> CameraFollow -> LightSync -> HierarchyTransform -> DebugOverlay -> DebugColliderRender -> RenderSync
```

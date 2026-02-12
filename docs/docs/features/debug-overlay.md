# Debug Overlay

The engine includes a debug overlay that displays real-time performance and scene information rendered directly on the GPU using a second Vulkan graphics pipeline.

## What It Shows

The overlay renders a semi-transparent dark panel at the top-left corner with three lines of green monospace text:

- **FPS** — Smoothed frames per second (exponential moving average: `0.95 * old + 0.05 * new`)
- **DT** — Delta time in milliseconds
- **Entities** — Number of active entities in the renderer

## Toggle

Press **F3** to toggle the overlay on or off. The overlay is **enabled by default** when `GameConstants.Debug` is `true`.

```csharp
// DebugOverlaySystem handles the F3 toggle automatically.
// You can also control it directly:
NativeBridge.SetDebugOverlay(true);   // show
NativeBridge.SetDebugOverlay(false);  // hide
```

## How It Works

### UI Rendering Pipeline

The overlay uses a dedicated Vulkan graphics pipeline separate from the 3D scene pipeline:

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
3. `recordUICommands()` binds the UI pipeline and draws within the same render pass

### Zero Overhead When Disabled

When the overlay is off, no geometry is built and no draw calls are recorded.

## DebugOverlaySystem

The `DebugOverlaySystem` runs each frame and handles:

- Edge-detected F3 key press to toggle `GameConstants.Debug`
- Calling `NativeBridge.SetDebugOverlay()` to sync the state to C++

It runs just before `RenderSyncSystem` in the system chain:

```
InputMovement -> Timer -> FreeCamera -> CameraFollow -> LightSync -> HierarchyTransform -> DebugOverlay -> RenderSync
```

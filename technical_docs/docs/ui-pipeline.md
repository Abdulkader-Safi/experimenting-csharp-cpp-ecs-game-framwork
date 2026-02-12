# UI Overlay Pipeline

## UIVertex Format

```cpp
struct UIVertex {
  glm::vec2 pos;   // pixel coordinates (0,0 = top-left)
  glm::vec2 uv;    // texture coordinates into font atlas
  glm::vec4 color; // RGBA (alpha used for transparency)
};
```

3 attribute descriptions, 32-byte stride.

## Pipeline Differences from 3D

| Setting        | 3D Pipeline            | UI Pipeline                     |
| -------------- | ---------------------- | ------------------------------- |
| Depth test     | ON (LESS)              | OFF                             |
| Depth write    | ON                     | OFF                             |
| Culling        | BACK                   | NONE                            |
| Blending       | OFF                    | SRC_ALPHA / ONE_MINUS_SRC_ALPHA |
| Push constants | mat4 model (64 bytes)  | vec2 screenSize (8 bytes)       |
| Descriptor set | UBO + Light + Material | Font atlas sampler              |

The UI pipeline renders within the **same render pass** as the 3D pipeline, after all 3D entities, before `vkCmdEndRenderPass`.

## Push Constants

```cpp
struct UIPushConstants {
  glm::vec2 screenSize; // 8 bytes
};
```

Used by the vertex shader to convert pixel coordinates to NDC.

## Font Atlas

Created in `createFontResources()`:

1. Load `assets/fonts/RobotoMono-Regular.ttf` via `stb_truetype`
2. Bake ASCII 32-126 (95 glyphs) at 20px height into a 512x512 bitmap
3. Upload as `VK_FORMAT_R8_UNORM` texture (single-channel)
4. Store glyph metrics in `glyphs_[95]` array (UV coords, advance, offset)

If the font file is missing, falls back to a 1x1 white placeholder (UI draws solid color quads only).

Font sampler uses `LINEAR` filtering and `CLAMP_TO_EDGE` addressing.

## Vertex Shader (`ui.vert`)

```glsl
#version 450

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main() {
    vec2 ndc = (inPos / pc.screenSize) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
    fragColor = inColor;
}
```

Converts pixel coordinates `[0, screenSize]` to NDC `[-1, 1]`. Z is always 0 (no depth).

## Fragment Shader (`ui.frag`)

```glsl
#version 450

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    float alpha = texture(fontAtlas, fragUV).r;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
```

Samples the R8 atlas for alpha coverage, multiplies by vertex color alpha. For non-text quads (background panels), the UV maps to the white region and alpha = 1.0.

## Debug Overlay Geometry

`buildDebugOverlayGeometry()` constructs the overlay each frame:

1. Clears `uiVertices_` vector
2. Smooths FPS with exponential moving average: `smoothedFps_ = 0.95 * smoothedFps_ + 0.05 * (1/dt)`
3. Calls `appendQuad()` for the background panel (semi-transparent dark)
4. Calls `appendText()` for each line: FPS, entity count, camera position, camera target
5. Copies final vertices into the persistently-mapped host-visible buffer for the current frame

### appendQuad(x, y, w, h, color)

Generates 6 vertices (2 triangles) for a solid-color rectangle. Uses UV (0,0) which maps to a white pixel in the atlas, so color comes entirely from vertex color.

### appendText(text, x, y, color)

For each character, looks up `glyphs_[c - 32]` to get atlas UVs and metrics, then emits 6 vertices (2 triangles) with proper positioning using `xoff`, `yoff`, and `xadvance`.

## Host-Visible Vertex Buffers

UI uses `UI_MAX_VERTICES = 4096` per frame, with one buffer per frame-in-flight (2 total). These are:

- `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | HOST_COHERENT_BIT`
- Persistently mapped at init (never unmapped)
- Written to directly via `memcpy` each frame

This avoids staging buffer overhead since UI geometry changes every frame.

## Recording UI Commands

`recordUICommands(commandBuffer)`:

1. Copy `uiVertices_` to mapped buffer: `memcpy(uiVertexBuffersMapped_[currentFrame_], ...)`
2. Bind UI pipeline
3. Set viewport and scissor (same as 3D)
4. Bind UI vertex buffer
5. Bind UI descriptor set (font atlas)
6. Push screenSize constant
7. `vkCmdDraw(uiVertexCount_, 1, 0, 0)` -- single non-indexed draw call

:::tip Where to Edit
**Adding new UI elements**: Add geometry generation in `buildDebugOverlayGeometry()` using `appendQuad()` for rectangles and `appendText()` for text. Keep total vertices under 4096.

**Changing font size**: Modify `fontPixelHeight_` (default 20.0f) in `renderer.h`. The atlas is baked at init, so this is a compile-time change.

**Adding new text lines**: Add `snprintf` + `appendText()` calls in `buildDebugOverlayGeometry()`. Increment the Y position by `fontPixelHeight_` for each line.
:::

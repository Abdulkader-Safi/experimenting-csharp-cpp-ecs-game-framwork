---
sidebar_position: 1
---

# Text Rendering

:::tip Implemented
The core text rendering system has been implemented as part of the [Debug Overlay](../../features/debug-overlay.md). The same pipeline can be extended for general-purpose UI text.
:::

Draw text on screen for health, score, and debug info.

## What's Done

- Font loading (TTF via stb_truetype)
- Bitmap font atlas generation (512x512, ASCII 32-126)
- Screen-space text rendering (orthographic projection via push constant)
- Configurable color per text call

## Remaining (Future)

- Configurable font size (currently fixed at 20px)
- Text alignment (left, center, right)
- Multiple fonts simultaneously
- Unicode support beyond ASCII

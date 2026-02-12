---
sidebar_position: 4
---

# Debug Overlay

:::tip Implemented
This feature has been implemented. See the [Debug Overlay feature page](../../features/debug-overlay.md) for full documentation.
:::

FPS counter, delta time, and entity count rendered as GPU text on a semi-transparent panel.

## What's Done

- FPS counter with exponential moving average smoothing
- Delta time display in milliseconds
- Active entity count
- F3 key toggle (edge-detected)
- Second Vulkan pipeline (no depth test, alpha blending)
- Font atlas via stb_truetype (512x512 R8_UNORM)
- Graceful fallback when font file is missing

## Remaining (Future)

- Wireframe rendering toggle
- Light visualization (show positions and directions)
- Component count display
- Custom overlay panels

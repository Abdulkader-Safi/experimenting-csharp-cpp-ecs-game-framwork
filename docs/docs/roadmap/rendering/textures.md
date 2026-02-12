---
sidebar_position: 1
---

# Textures & UV Mapping

:::tip Implemented
This feature has been implemented. See the [Textures & UV Mapping](../../features/textures.md) feature documentation.
:::

Load textures from glTF files, sample them in the fragment shader, and add UV coordinates to the vertex format.

## What Was Delivered

- UV coordinates (`TEXCOORD_0`) extracted from glTF and generated for all procedural primitives
- Base color texture loading from glTF (embedded GLB and external URI)
- Per-material descriptor set (set 1) with `COMBINED_IMAGE_SAMPLER`
- 1x1 white fallback texture for untextured meshes
- Fragment shader: `texture(baseColorTex, fragUV).rgb * vertexColor` before Blinn-Phong lighting
- PNG/JPEG decoding via stb_image

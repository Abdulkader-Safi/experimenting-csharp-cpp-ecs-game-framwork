# Textures & UV Mapping

The engine supports texture mapping for glTF models and procedural primitives. Textures are loaded from glTF files (embedded or external), decoded with stb_image, and sampled in the fragment shader. The final pixel color is `textureColor * vertexColor * lighting`.

## How It Works

The renderer uses a two-descriptor-set layout:

- **Set 0** — Global UBOs (view/projection matrix, light data)
- **Set 1** — Per-material texture (a `COMBINED_IMAGE_SAMPLER` bound per draw call)

Every mesh has a `materialId` pointing to a material entry. Materials that don't have a texture use a 1x1 white fallback, so `white * vertexColor = vertexColor` — existing flat-colored meshes render identically.

## glTF Texture Loading

When loading a `.glb` model with `NativeBridge.LoadMesh()`, the engine automatically:

1. Extracts `TEXCOORD_0` UV coordinates from the mesh
2. Reads `base_color_texture` from the PBR metallic-roughness material
3. Decodes the image (PNG/JPEG) via stb_image
4. Uploads to a `VK_FORMAT_R8G8B8A8_SRGB` Vulkan image
5. Creates a material with a descriptor set pointing to the texture + sampler

```csharp
// Textured models work the same as before — textures load automatically
int meshId = NativeBridge.LoadMesh("models/TexturedBox.glb");
int entity = NativeBridge.CreateEntity(meshId);
```

Both embedded textures (GLB buffer views) and external URI references (relative file paths) are supported. If texture loading fails, the mesh falls back to flat vertex colors.

## Untextured Models

Models without a `base_color_texture` continue to use `base_color_factor` as vertex color. The 1x1 white fallback texture ensures no visual change:

```csharp
// Still works — renders with base_color_factor vertex color
int meshId = NativeBridge.LoadMesh("models/UntexturedBox.glb");
```

## Procedural Primitive UVs

All procedural primitives generate UV coordinates:

| Shape        | UV Mapping                                                     |
| ------------ | -------------------------------------------------------------- |
| **Box**      | Each face maps `(0,0)` to `(1,1)` independently                |
| **Sphere**   | Spherical mapping: `u = seg/segments`, `v = ring/rings`        |
| **Plane**    | Four corners: `(0,0)`, `(1,0)`, `(1,1)`, `(0,1)`               |
| **Cylinder** | Side: cylindrical wrap. Caps: radial projection to unit circle |
| **Capsule**  | Parametric: `u = seg/segments`, `v = row/totalRows`            |

Procedural primitives always use the default white material (flat vertex color), but the UVs are available for future use with custom materials.

## Texture Sampler

The texture sampler uses:

- **Filter**: `VK_FILTER_LINEAR` (bilinear filtering)
- **Address mode**: `VK_SAMPLER_ADDRESS_MODE_REPEAT` (tiling)
- **Format**: `VK_FORMAT_R8G8B8A8_SRGB` (sRGB color space)

## Fragment Shader

The fragment shader samples the texture and multiplies with vertex color before lighting:

```glsl
vec4 texColor = texture(baseColorTex, fragUV);
vec3 baseColor = texColor.rgb * fragColor;
// Blinn-Phong lighting applied to baseColor...
```

This means:

- Textured models: texture color modulated by `base_color_factor`
- Untextured models: `white (1,1,1) * vertexColor = vertexColor` (no change)

## Limitations

- Only the first `base_color_texture` across all primitives in a mesh is loaded (one material per mesh)
- No mipmap generation (single mip level)
- No normal maps, metallic-roughness maps, or other PBR textures yet (see [PBR Materials roadmap](../roadmap/rendering/pbr-materials.md))

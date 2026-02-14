# Materials & Textures

## Material System Overview

Each material is a `MaterialData` struct holding GPU texture resources and a descriptor set:

```cpp
struct MaterialData {
  VkImage textureImage;
  VkDeviceMemory textureMemory;
  VkImageView textureView;
  VkDescriptorSet descriptorSet;
  bool ownsTexture;
};
```

Materials are stored in `materials_` vector. Material ID 0 is always the default (1x1 white) texture.

Maximum materials: `MAX_MATERIALS = 64` (descriptor pool limit).

## Descriptor Set 1 (Per-Material)

```
Layout: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER at binding 0
Stage:  VK_SHADER_STAGE_FRAGMENT_BIT
```

Bound per-entity in the draw loop:

```cpp
VkDescriptorSet matSet = materials_[mesh.materialId].descriptorSet;
vkCmdBindDescriptorSets(cmd, GRAPHICS, pipelineLayout_, 1, 1, &matSet, 0, nullptr);
```

## Default Texture

`createDefaultTexture()` creates a 1x1 white RGBA pixel (`{255, 255, 255, 255}`) as `VK_FORMAT_R8G8B8A8_SRGB`. This is used for meshes without textures — the fragment shader multiplies `texture(baseColorTex, fragUV).rgb * fragColor`, so a white texture means vertex color passes through unchanged.

## Texture Sampler

`createTextureSampler()` creates a shared sampler:

- **Filter**: `VK_FILTER_LINEAR` (mag + min)
- **Address mode**: `VK_SAMPLER_ADDRESS_MODE_REPEAT` (all axes)
- **Anisotropy**: disabled
- **Mipmap mode**: `VK_SAMPLER_MIPMAP_MODE_LINEAR`
- **Format**: textures use `VK_FORMAT_R8G8B8A8_SRGB`

## loadTextureFromMemory()

`loadTextureFromMemory(const uint8_t* data, size_t size)` loads a texture from encoded image data (PNG, JPG, etc.):

1. **Decode**: `stbi_load_from_memory()` → RGBA pixels, forced 4 channels
2. **Create image**: `VK_FORMAT_R8G8B8A8_SRGB`, optimal tiling, device-local
3. **Upload**: staging buffer → `transitionImageLayout` (UNDEFINED → TRANSFER_DST) → `copyBufferToImage` → `transitionImageLayout` (TRANSFER_DST → SHADER_READ_ONLY)
4. **Create image view**: standard 2D view with COLOR aspect
5. **Create material**: allocates descriptor set, writes sampler + image view
6. Returns material ID (index into `materials_`)

## glTF Texture Extraction

In `loadMesh()`, after parsing geometry, the loader scans for `base_color_texture`:

**Embedded (GLB)**:

```cpp
const uint8_t* texData = (uint8_t*)image->buffer_view->buffer->data
                        + image->buffer_view->offset;
size_t texSize = image->buffer_view->size;
meshMaterialId = loadTextureFromMemory(texData, texSize);
```

**External URI**:

```cpp
string dir = gltfPath.substr(0, gltfPath.find_last_of("/\\") + 1);
string texPath = dir + image->uri;
// Read file, then loadTextureFromMemory(fileData, fileSize)
```

Only the first material with a base color texture is loaded. If no texture is found, the mesh uses `defaultMaterialId_` (1x1 white).

## Cleanup

`cleanupMaterialResources()` destroys:

1. All materials that `ownsTexture` — destroys image view, image, and frees memory
2. Default texture (image view, image, memory)
3. Texture sampler
4. Material descriptor pool
5. Material descriptor set layout

:::tip Where to Edit
**Adding PBR maps** (normal, metallic-roughness, etc.): Add additional `VkImage`/`VkImageView` fields to `MaterialData`. Expand the descriptor set layout to include new `COMBINED_IMAGE_SAMPLER` bindings (binding 1, 2, etc.). Load additional textures in `loadMesh()` and update `shader.frag` to sample them.

**Changing sampler settings**: Modify `createTextureSampler()` in `renderer.cpp`. For example, enable anisotropy with `anisotropyEnable = VK_TRUE` and set `maxAnisotropy` (query device limits first).

**Adding material properties**: Add fields to `MaterialData` (e.g., roughness, metallic values). To make them available in the shader, either expand the material descriptor set with a UBO or use push constants.
:::

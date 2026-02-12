# Structs & Data Layout

## 3D Vertex

```cpp
struct Vertex {
  glm::vec3 pos;    // location 0, R32G32B32_SFLOAT
  glm::vec3 normal; // location 1, R32G32B32_SFLOAT
  glm::vec3 color;  // location 2, R32G32B32_SFLOAT
  glm::vec2 uv;     // location 3, R32G32_SFLOAT
};
```

Total stride: 44 bytes. Used for all 3D geometry (loaded meshes + procedural primitives).

## UI Vertex

```cpp
struct UIVertex {
  glm::vec2 pos;   // location 0, R32G32_SFLOAT (pixel coordinates)
  glm::vec2 uv;    // location 1, R32G32_SFLOAT
  glm::vec4 color; // location 2, R32G32B32A32_SFLOAT (RGBA with alpha)
};
```

Total stride: 32 bytes. Used for debug overlay text and background quads.

## GlyphInfo

```cpp
struct GlyphInfo {
  float x0, y0, x1, y1; // UV coordinates in font atlas
  float xoff, yoff, xadvance;
  float width, height;   // pixel dimensions
};
```

Stores baked glyph metrics from stb_truetype. Array of 95 entries (ASCII 32-126).

## Push Constants

### 3D Push Constants

```cpp
struct PushConstantData {
  glm::mat4 model; // 64 bytes
};
```

Sent per-entity via `vkCmdPushConstants` to the vertex shader.

### UI Push Constants

```cpp
struct UIPushConstants {
  glm::vec2 screenSize; // 8 bytes
};
```

Sent once per UI draw to convert pixel coords to NDC.

## Uniform Buffer Objects

### View/Projection UBO (set 0, binding 0)

```cpp
struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};
```

Updated per frame. Projection Y-axis is flipped for Vulkan (`proj[1][1] *= -1`).

### Light UBO (set 0, binding 1)

```cpp
struct GpuLight {
  alignas(16) glm::vec4 position;   // xyz = pos, w = unused
  alignas(16) glm::vec4 direction;  // xyz = dir, w = unused
  alignas(16) glm::vec4 color;      // xyz = RGB, w = intensity
  alignas(4)  float innerCone;      // cos(inner angle) for spot
  alignas(4)  float outerCone;      // cos(outer angle) for spot
  alignas(4)  float radius;         // attenuation radius (0 = infinite)
  alignas(4)  int type;             // 0=directional, 1=point, 2=spot
};
// sizeof(GpuLight) = 64 bytes

struct LightUBO {
  alignas(16) glm::vec4 cameraPos;  // xyz = eye position
  alignas(4)  int numLights;
  alignas(4)  float ambientIntensity;
  alignas(8)  float _pad[2];        // pad to 16 before array
  GpuLight lights[MAX_LIGHTS];      // 8 x 64 = 512 bytes
};
```

`alignas` annotations are critical for std140/std430 layout compatibility.

## Material Data

```cpp
struct MaterialData {
  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureMemory = VK_NULL_HANDLE;
  VkImageView textureView = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  bool ownsTexture = true;
};
```

Each material owns its GPU texture resources and a descriptor set bound to pipeline set 1.

## Mesh Data

```cpp
struct MeshData {
  int32_t vertexOffset;   // offset into allVertices_
  uint32_t indexOffset;   // offset into allIndices_
  uint32_t indexCount;    // number of indices for this mesh
  int materialId = 0;     // index into materials_ array
};
```

## Entity Data

```cpp
struct EntityData {
  int meshId;
  glm::mat4 transform;
  bool active;
};
```

## Queue Family Indices

```cpp
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() const;
};
```

## Combined Buffer Strategy

All mesh vertices are concatenated into a single `allVertices_` vector and all indices into `allIndices_`. Each `MeshData` stores its `vertexOffset` and `indexOffset` into these combined arrays. At draw time, `vkCmdDrawIndexed` uses these offsets:

```cpp
vkCmdDrawIndexed(cmd, mesh.indexCount, 1, mesh.indexOffset, mesh.vertexOffset, 0);
```

This means a single vertex buffer bind + single index buffer bind serves all entities. New meshes are appended and `buffersNeedRebuild_` is set to true, triggering `rebuildGeometryBuffers()` on the next frame (staging buffer -> device-local copy).

:::tip Where to Edit
**Adding a new vertex attribute**: Add the field to `Vertex` in `renderer.h`, add a new `VkVertexInputAttributeDescription` in `getAttributeDescriptions()`, update the array size, and update `shader.vert` to declare the new input/output.

**Changing UBO layout**: Modify the struct in `renderer.h`, ensure `alignas` matches std140 rules, update the corresponding GLSL `uniform` block, and verify `sizeof()` matches between C++ and shader.
:::

# 3D Scene Pipeline

## Descriptor Set Layout

The 3D pipeline uses two descriptor sets:

**Set 0** (per-frame, shared across all entities):

- Binding 0: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` — `UniformBufferObject` (view/proj matrices), vertex stage
- Binding 1: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` — `LightUBO` (camera pos + up to 8 lights), fragment stage

**Set 1** (per-material):

- Binding 0: `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` — base color texture, fragment stage

## Graphics Pipeline State

Created in `createGraphicsPipeline()`:

- **Vertex input**: `Vertex` binding (pos, normal, color, uv) — 4 attribute descriptions
- **Topology**: `TRIANGLE_LIST`
- **Rasterizer**: fill mode, 1.0 line width, `BACK` culling, `COUNTER_CLOCKWISE` front face
- **Depth/stencil**: depth test ON, depth write ON, compare op `LESS`
- **Color blend**: blending OFF (opaque), all RGBA write mask
- **Multisampling**: `SAMPLE_COUNT_1_BIT` (no MSAA)
- **Dynamic state**: viewport + scissor (set per frame in `recordCommandBuffer`)

## Push Constants

A 64-byte `PushConstantData` (one `mat4 model`) is pushed per entity via `vkCmdPushConstants`. This carries the entity's world transform, avoiding per-entity UBO updates.

```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(PushConstantData); // 64 bytes
```

## Pipeline Layout

Two descriptor set layouts + one push constant range:

```
Set 0: [UBO (view/proj), LightUBO]
Set 1: [Material texture sampler]
Push:  [mat4 model — 64 bytes, vertex stage]
```

## Vertex Shader (`shader.vert`)

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragUV;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    fragColor = inColor;
    fragWorldPos = worldPos.xyz;
    fragUV = inUV;
}
```

Key points:

- Normal transform: `mat3(transpose(inverse(model)))` handles non-uniform scaling correctly
- World position is passed to fragment shader for per-fragment lighting
- Projection already has Y-flip applied on CPU side (`proj[1][1] *= -1`)

## Fragment Shader (`shader.frag`)

The fragment shader implements Blinn-Phong shading with up to 8 dynamic lights.

The `calcLight()` function handles all three light types:

- **Directional**: no attenuation, uses `normalize(-direction)` as light direction
- **Point**: distance-based quadratic attenuation `(1 - d^2/r^2)^2`, position-relative direction
- **Spot**: same as point + cone falloff `clamp((theta - outerCone) / (innerCone - outerCone))`

Lighting equation per light:

```
diffuse  = max(dot(N, L), 0.0)
specular = pow(max(dot(N, H), 0.0), 32.0)   // H = normalize(L + V)
result   = (diffuse + specular) * lightColor * intensity * attenuation
```

Final color = `ambient + sum(baseColor * calcLight(light[i]))` where `ambient = baseColor * ambientIntensity`.

If `numLights == 0`, falls back to a hardcoded directional light at `(1, 1, 1)` with 0.15 ambient.

Texture sampling: `texture(baseColorTex, fragUV).rgb * fragColor` — the texture color is multiplied by the vertex color.

## Debug Wireframe Pipeline

A second pipeline (`debugPipeline_`) is created at the end of `createGraphicsPipeline()` by modifying the rasterizer and depth-stencil state:

| Setting       | 3D Pipeline             | Debug Wireframe Pipeline      |
| ------------- | ----------------------- | ----------------------------- |
| Polygon mode  | `VK_POLYGON_MODE_FILL`  | `VK_POLYGON_MODE_LINE`        |
| Cull mode     | `VK_CULL_MODE_BACK_BIT` | `VK_CULL_MODE_NONE`           |
| Depth write   | `VK_TRUE`               | `VK_FALSE`                    |
| Depth compare | `VK_COMPARE_OP_LESS`    | `VK_COMPARE_OP_LESS_OR_EQUAL` |

The wireframe pipeline shares the same shaders, pipeline layout, vertex format, and vertex/index buffers as the 3D pipeline. It requires the `fillModeNonSolid` device feature, which is enabled during logical device creation.

Debug entities are stored in a separate `debugEntities_` vector and rendered between the main 3D entities and the UI overlay in `recordCommandBuffer()`.

:::tip Where to Edit
**Adding a new uniform**: Add the field to the UBO struct in `renderer.h` (with correct `alignas`), update the matching GLSL `uniform` block, and ensure the buffer size matches.

**Modifying lighting**: Edit `calcLight()` in `shader.frag`. The specular exponent (32.0) is hardcoded — to make it configurable, add it to `GpuLight` or as a material property.

**Adding a vertex attribute**: Add to `Vertex` struct in `renderer.h`, add a `VkVertexInputAttributeDescription`, increment the array size, declare in `shader.vert`, and pass through to `shader.frag` if needed.
:::

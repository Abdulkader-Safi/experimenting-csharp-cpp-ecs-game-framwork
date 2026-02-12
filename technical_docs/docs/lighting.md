# Lighting System

## GpuLight Struct

```cpp
struct GpuLight {
  alignas(16) glm::vec4 position;   // xyz = world position, w = unused
  alignas(16) glm::vec4 direction;  // xyz = direction, w = unused
  alignas(16) glm::vec4 color;      // xyz = RGB color, w = intensity
  alignas(4)  float innerCone;      // cos(inner angle) for spot lights
  alignas(4)  float outerCone;      // cos(outer angle) for spot lights
  alignas(4)  float radius;         // attenuation radius (0 = infinite/no falloff)
  alignas(4)  int type;             // 0 = directional, 1 = point, 2 = spot
};
```

Each light is 64 bytes (std140 compatible).

## LightUBO Layout

```cpp
struct LightUBO {
  alignas(16) glm::vec4 cameraPos;  // xyz = camera eye (for specular)
  alignas(4)  int numLights;         // active light count
  alignas(4)  float ambientIntensity; // global ambient (default 0.15)
  alignas(8)  float _pad[2];         // pad to 16-byte boundary before array
  GpuLight lights[MAX_LIGHTS];       // 8 lights x 64 bytes = 512 bytes
};
```

Bound at descriptor set 0, binding 1. Updated every frame in `updateUniformBuffer()`.

## C++ API

### setLight()

```cpp
void setLight(int index, int type, float posX, float posY, float posZ,
              float dirX, float dirY, float dirZ,
              float r, float g, float b, float intensity,
              float radius, float innerCone, float outerCone);
```

Sets a light at `index` (0-7). Automatically recalculates `numLights` as the highest active index + 1 (determined by `color.w > 0`, i.e. intensity > 0).

### clearLight()

Zeros the light at `index` and recalculates `numLights`.

### setAmbientIntensity()

Sets `lightData_.ambientIntensity`. Default is 0.15.

## Fragment Shader Algorithm

### calcLight() (per-light)

```glsl
vec3 calcLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    float intensity = light.color.w;
    vec3  lightColor = light.color.rgb;
    vec3  lightDir;
    float attenuation = 1.0;

    if (light.type == LIGHT_DIRECTIONAL) {
        lightDir = normalize(-light.direction.xyz);
    } else {
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        lightDir = toLight / dist;

        if (light.radius > 0.0) {
            attenuation = clamp(1.0 - (dist*dist) / (light.radius*light.radius), 0.0, 1.0);
            attenuation *= attenuation; // squared falloff
        }

        if (light.type == LIGHT_SPOT) {
            float theta = dot(lightDir, normalize(-light.direction.xyz));
            float epsilon = light.innerCone - light.outerCone;
            float spotFactor = clamp((theta - light.outerCone) / epsilon, 0.0, 1.0);
            attenuation *= spotFactor;
        }
    }

    // Diffuse (Lambertian)
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

    return (diff + spec) * lightColor * intensity * attenuation;
}
```

### Light Types

**Directional** (`type = 0`):

- No position, uses direction only
- No attenuation (infinite distance)
- Light direction = `normalize(-direction)`
- Use for sun/moon

**Point** (`type = 1`):

- Has position, no direction used
- Quadratic attenuation: `(1 - d^2/r^2)^2`
- radius = 0 means no falloff (infinite range)
- Use for light bulbs, torches

**Spot** (`type = 2`):

- Has position AND direction
- Same attenuation as point, plus cone falloff
- `innerCone`/`outerCone` are **cosines** of the cone angles (pass `cos(radians(angle))`)
- Smooth falloff between inner and outer cone
- Use for flashlights, stage lights

### Main Shader

```glsl
void main() {
    // If no lights configured, use hardcoded fallback
    if (ld.numLights == 0) {
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diffuse = max(dot(normal, lightDir), 0.0);
        outColor = vec4(baseColor * (0.15 + diffuse), 1.0);
        return;
    }

    vec3 viewDir = normalize(ld.cameraPos.xyz - fragWorldPos);
    vec3 result = baseColor * ld.ambientIntensity; // ambient term

    for (int i = 0; i < ld.numLights; i++) {
        result += baseColor * calcLight(ld.lights[i], normal, fragWorldPos, viewDir);
    }

    outColor = vec4(result, 1.0);
}
```

## MAX_LIGHTS Constraint

The hard limit is `MAX_LIGHTS = 8`, defined in both `renderer.h` and `shader.frag`. The light array is fixed-size in the UBO (512 bytes). Only `numLights` entries are evaluated in the shader loop.

:::tip Where to Edit
**Adding shadow maps**: Create a depth-only render pass for each shadow-casting light. Render the scene from the light's perspective. Pass the resulting depth textures to the fragment shader as additional samplers and perform PCF shadow testing in `calcLight()`.

**Changing attenuation formula**: Modify the `attenuation` calculation in `calcLight()`. The current formula is `(1 - d^2/r^2)^2`. For inverse-square falloff, use `1.0 / (1.0 + d*d)`.

**Increasing the light count**: Change `MAX_LIGHTS` in both `renderer.h` and `shader.frag`. Update the `_pad` array in `LightUBO` if alignment changes. Note that more lights linearly increases per-fragment cost.
:::

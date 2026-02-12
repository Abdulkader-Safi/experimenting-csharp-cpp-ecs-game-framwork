#version 450

#define MAX_LIGHTS 8

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

struct Light {
    vec4  position;   // xyz = pos
    vec4  direction;  // xyz = dir
    vec4  color;      // xyz = RGB, w = intensity
    float innerCone;  // cos(inner angle)
    float outerCone;  // cos(outer angle)
    float radius;     // attenuation radius (0 = infinite)
    int   type;       // 0=dir, 1=point, 2=spot
};

layout(set = 0, binding = 1) uniform LightData {
    vec4  cameraPos;       // xyz = eye position
    int   numLights;
    float ambientIntensity;
    // 8 bytes implicit std140 padding before array
    Light lights[MAX_LIGHTS];
} ld;

layout(set = 1, binding = 0) uniform sampler2D baseColorTex;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

vec3 calcLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    float intensity = light.color.w;
    vec3  lightColor = light.color.rgb;
    vec3  lightDir;
    float attenuation = 1.0;

    if (light.type == LIGHT_DIRECTIONAL) {
        lightDir = normalize(-light.direction.xyz);
    } else {
        // Point or spot
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        lightDir = toLight / dist;

        if (light.radius > 0.0) {
            attenuation = clamp(1.0 - (dist * dist) / (light.radius * light.radius), 0.0, 1.0);
            attenuation *= attenuation;
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

void main() {
    vec3 normal = normalize(fragNormal);
    vec4 texColor = texture(baseColorTex, fragUV);
    vec3 baseColor = texColor.rgb * fragColor;

    if (ld.numLights == 0) {
        // Fallback: original hardcoded lighting
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diffuse = max(dot(normal, lightDir), 0.0);
        float ambient = 0.15;
        outColor = vec4(baseColor * (ambient + diffuse), 1.0);
        return;
    }

    vec3 viewDir = normalize(ld.cameraPos.xyz - fragWorldPos);

    vec3 ambient = baseColor * ld.ambientIntensity;
    vec3 result = ambient;

    for (int i = 0; i < ld.numLights; i++) {
        result += baseColor * calcLight(ld.lights[i], normal, fragWorldPos, viewDir);
    }

    outColor = vec4(result, 1.0);
}

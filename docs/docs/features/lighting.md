---
sidebar_position: 2
---

# Lighting

The engine supports up to 8 dynamic lights using Blinn-Phong shading. Lights are ECS entities with a `Light` component and a `Transform` for position.

## Light Types

### Directional Light

A light that shines in a single direction from infinitely far away (like the sun). Position is ignored; only direction matters.

```csharp
int sun = world.Spawn();
world.AddComponent(sun, new Transform());
world.AddComponent(sun, new Light {
    Type = Light.Directional,
    DirX = 0.2f, DirY = -1f, DirZ = 0.3f,
    ColorR = 1f, ColorG = 0.95f, ColorB = 0.8f,
    Intensity = 1.0f
});
```

### Point Light

A light that emits in all directions from a position. Attenuates with distance based on `Radius`.

```csharp
int torch = world.Spawn();
world.AddComponent(torch, new Transform { X = 2f, Y = 3f, Z = -1f });
world.AddComponent(torch, new Light {
    Type = Light.Point,
    ColorR = 1f, ColorG = 0.6f, ColorB = 0.2f,
    Intensity = 2.0f,
    Radius = 15f
});
```

### Spot Light

A cone-shaped light from a position in a direction. Uses inner and outer cone angles for smooth falloff.

```csharp
int spotlight = world.Spawn();
world.AddComponent(spotlight, new Transform { X = 0f, Y = 5f, Z = 0f });
world.AddComponent(spotlight, new Light {
    Type = Light.Spot,
    DirX = 0f, DirY = -1f, DirZ = 0f,
    ColorR = 1f, ColorG = 1f, ColorB = 1f,
    Intensity = 3.0f,
    Radius = 20f,
    InnerConeDeg = 12.5f,
    OuterConeDeg = 17.5f
});
```

## Ambient Light

Control the base ambient lighting level:

```csharp
NativeBridge.SetAmbient(0.1f);  // dim ambient
```

## LightSyncSystem

The `LightSyncSystem` runs each frame and pushes all `Light` + `Transform` entities to the renderer. It:
1. Queries all entities with both components
2. Assigns each light a slot (0–7)
3. Clears unused slots

Lights beyond slot 7 are ignored. The system automatically manages slot assignment — you don't need to set `LightIndex` manually.

## Light Properties Reference

| Field | Type | Default | Description |
|---|---|---|---|
| `Type` | `int` | `Directional` (0) | `Light.Directional`, `Light.Point`, or `Light.Spot` |
| `ColorR/G/B` | `float` | `1, 1, 1` | RGB color |
| `Intensity` | `float` | `1` | Brightness multiplier |
| `DirX/Y/Z` | `float` | `0, -1, 0` | Direction (directional and spot) |
| `Radius` | `float` | `10` | Attenuation radius (point and spot) |
| `InnerConeDeg` | `float` | `12.5` | Inner cone angle in degrees (spot only) |
| `OuterConeDeg` | `float` | `17.5` | Outer cone angle in degrees (spot only) |
| `LightIndex` | `int` | `-1` | Auto-assigned slot index |

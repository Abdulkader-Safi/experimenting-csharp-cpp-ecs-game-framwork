# Lighting

The engine supports up to 8 dynamic lights using Blinn-Phong shading. Lights are ECS entities with a `Light` component and a `Transform` for position.

## Light Types

### Directional Light

A light that shines in a single direction from infinitely far away (like the sun). Position is ignored; only direction matters.

```csharp
int sun = world.Spawn(
    new Transform(),
    new Light {
        Type = LightType.Directional,
        Direction = new Vec3(0.2f, -1f, 0.3f),
        LightColor = new Color(1f, 0.95f, 0.8f),
        Intensity = 1.0f
    }
);
```

### Point Light

A light that emits in all directions from a position. Attenuates with distance based on `Radius`.

```csharp
int torch = world.Spawn(
    new Transform { Position = new Vec3(2f, 3f, -1f) },
    new Light {
        Type = LightType.Point,
        LightColor = new Color(1f, 0.6f, 0.2f),
        Intensity = 2.0f,
        Radius = 15f
    }
);
```

### Spot Light

A cone-shaped light from a position in a direction. Uses inner and outer cone angles for smooth falloff.

```csharp
int spotlight = world.Spawn(
    new Transform { Position = new Vec3(0f, 5f, 0f) },
    new Light {
        Type = LightType.Spot,
        Direction = new Vec3(0f, -1f, 0f),
        LightColor = Color.White,
        Intensity = 3.0f,
        Radius = 20f,
        InnerConeDeg = 12.5f,
        OuterConeDeg = 17.5f
    }
);
```

## Ambient Light

Control the base ambient lighting level:

```csharp
NativeBridge.SetAmbient(0.1f);  // dim ambient
```

## LightSyncSystem

The `LightSyncSystem` runs each frame and pushes all `Light` + `Transform` entities to the renderer. It:

1. Queries all entities with both components
2. Assigns each light a slot (0-7)
3. Clears unused slots

Lights beyond slot 7 are ignored. The system automatically manages slot assignment — you don't need to set `_LightIndex` manually.

## Light Properties Reference

| Field          | Type        | Default                 | Description                                |
| -------------- | ----------- | ----------------------- | ------------------------------------------ |
| `Type`         | `LightType` | `LightType.Directional` | `Directional`, `Point`, or `Spot`          |
| `LightColor`   | `Color`     | `Color.White`           | Light color                                |
| `Intensity`    | `float`     | `1`                     | Brightness multiplier                      |
| `Direction`    | `Vec3`      | `(0, -1, 0)`            | Direction (directional and spot)           |
| `Radius`       | `float`     | `10`                    | Attenuation radius (point and spot)        |
| `InnerConeDeg` | `float`     | `12.5`                  | Inner cone angle in degrees (spot only)    |
| `OuterConeDeg` | `float`     | `17.5`                  | Outer cone angle in degrees (spot only)    |
| `_LightIndex`  | `int`       | `-1`                    | Auto-assigned slot index (engine-internal) |

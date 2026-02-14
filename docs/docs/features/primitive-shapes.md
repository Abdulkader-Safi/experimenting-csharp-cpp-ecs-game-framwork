# Primitive Shapes

The engine can generate procedural meshes for common 3D shapes without requiring external model files. Each primitive is centered at the origin with exact user-specified dimensions. Position, rotation, and scale are handled by the `Transform` component.

## Available Shapes

### Box

A six-faced cuboid with per-face normals.

```csharp
int meshId = NativeBridge.CreateBoxMesh(2f, 1f, 3f, new Color(0.8f, 0.2f, 0.2f));
```

| Parameter | Description                       |
| --------- | --------------------------------- |
| `w, h, l` | Width (X), height (Y), length (Z) |
| `Color c` | Vertex color                      |

### Sphere

A UV sphere with outward-pointing normals.

```csharp
int meshId = NativeBridge.CreateSphereMesh(0.5f, 32, 16, new Color(0.2f, 0.8f, 0.2f));
```

| Parameter  | Description                         |
| ---------- | ----------------------------------- |
| `radius`   | Sphere radius                       |
| `segments` | Horizontal subdivisions (longitude) |
| `rings`    | Vertical subdivisions (latitude)    |
| `Color c`  | Vertex color                        |

### Plane

A flat quad on the XZ plane at Y=0, with normal pointing up.

```csharp
int meshId = NativeBridge.CreatePlaneMesh(10f, 10f, new Color(0.3f, 0.8f, 0.3f));
```

| Parameter | Description             |
| --------- | ----------------------- |
| `w, h`    | Width (X) and depth (Z) |
| `Color c` | Vertex color            |

### Cylinder

A capped cylinder aligned along the Y axis.

```csharp
int meshId = NativeBridge.CreateCylinderMesh(0.4f, 1f, 32, new Color(0.2f, 0.2f, 0.8f));
```

| Parameter  | Description     |
| ---------- | --------------- |
| `radius`   | Cylinder radius |
| `height`   | Total height    |
| `segments` | Number of sides |
| `Color c`  | Vertex color    |

### Capsule

A cylinder with hemisphere caps. Total height = `height + 2 * radius`.

```csharp
int meshId = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f, 32, 16, new Color(0.9f, 0.8f, 0.2f));
```

| Parameter  | Description                          |
| ---------- | ------------------------------------ |
| `radius`   | Radius of caps and cylinder          |
| `height`   | Length of the cylinder portion       |
| `segments` | Horizontal subdivisions              |
| `rings`    | Vertical subdivisions per hemisphere |
| `Color c`  | Vertex color                         |

## Convenience Overloads

Every shape has a short form that uses default color (grey `0.7`) and default tessellation (32 segments, 16 rings):

```csharp
int box    = NativeBridge.CreateBoxMesh(1f, 1f, 1f);
int sphere = NativeBridge.CreateSphereMesh(0.5f);
int plane  = NativeBridge.CreatePlaneMesh(10f, 10f);
int cyl    = NativeBridge.CreateCylinderMesh(0.4f, 1f);
int cap    = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f);
```

## Usage with ECS

Primitive meshes work exactly like glTF meshes. The returned mesh ID can be shared across multiple entities:

```csharp
int boxMesh = NativeBridge.CreateBoxMesh(1f, 1f, 1f, new Color(0.8f, 0.2f, 0.2f));

// Spawn an entity with the box
int box = world.SpawnMeshEntity(boxMesh, new Transform {
    Position = new Vec3(0f, 0.5f, 0f)
});
```

:::note
Unlike glTF meshes, procedural primitives are **not** auto-centered or auto-scaled. They are generated at the exact dimensions you specify, centered at the origin.
:::

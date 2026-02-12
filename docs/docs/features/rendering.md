---
sidebar_position: 1
---

# Rendering

The engine uses Vulkan (via MoltenVK on macOS) for 3D rendering with support for glTF mesh loading and multi-entity draw calls.

## Mesh Loading

Load glTF models using `NativeBridge.LoadMesh()`. This parses the `.glb` file with cgltf and uploads vertex/index data to the GPU:

```csharp
int meshId = NativeBridge.LoadMesh("models/Box.glb");
```

The returned `meshId` identifies the geometry on the GPU. You can share it across multiple entities.

## Procedural Primitives

The engine can also generate meshes for common shapes (box, sphere, plane, cylinder, capsule) without any external files. See [Primitive Shapes](primitive-shapes.md) for the full API and examples.

## Multi-Entity Rendering

Each entity gets its own draw slot in the renderer. The model matrix is passed via Vulkan push constants:

```csharp
int entity1 = NativeBridge.CreateEntity(meshId);
int entity2 = NativeBridge.CreateEntity(meshId);  // same mesh, separate transform
```

The `RenderSyncSystem` automatically pushes each entity's `Transform.ToMatrix()` to its draw slot every frame.

## Render Pipeline

The engine has two Vulkan graphics pipelines sharing the same render pass:

### 3D Scene Pipeline

- **Vertex shader** — UBO for view/projection matrices, push constant for per-entity model matrix
- **Fragment shader** — Blinn-Phong shading with support for up to 8 dynamic lights
- **Depth testing** — enabled, ensures correct draw order
- **Back-face culling** — enabled, improves performance

### UI Overlay Pipeline

- **Vertex shader** — 2D pixel coordinates converted to NDC via `vec2 screenSize` push constant
- **Fragment shader** — Samples R8 font atlas texture, multiplies by vertex color alpha
- **No depth testing** — UI always renders on top of the 3D scene
- **Alpha blending** — `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` for transparent panels and anti-aliased text
- **No backface culling** — 2D quads rendered as triangle lists

The UI pipeline is used by the [debug overlay](debug-overlay.md) and can be extended for HUD elements, menus, and other screen-space UI.

## Supported Model Format

- **glTF Binary (.glb)** — positions, normals, and indices are loaded. UV coordinates and textures are not yet supported (see [Roadmap](../roadmap/rendering/textures.md)).

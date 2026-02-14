# Meshes & Geometry

## loadMesh() Flow

`loadMesh(const char* path)` loads a glTF/GLB file:

1. **Parse**: `cgltf_parse_file()` + `cgltf_load_buffers()` (header-only cgltf library)
2. **Extract geometry**: Iterates all meshes → primitives (triangles only). For each primitive:
   - Reads POSITION accessor (required)
   - Reads NORMAL accessor (optional, defaults to (0, 1, 0))
   - Reads TEXCOORD accessor (optional, defaults to (0, 0))
   - Reads base color factor from PBR metallic-roughness material (defaults to (0.7, 0.7, 0.8))
   - Reads indices (or generates sequential indices if unindexed)
3. **Load texture**: Scans primitives for `base_color_texture`:
   - Embedded (GLB): reads from `buffer_view->buffer->data + offset`
   - External URI: resolves relative to glTF file path, reads file
   - Calls `loadTextureFromMemory()` to create GPU texture + material
4. **Auto-center and scale**: Computes AABB, centers at origin, scales to fit within 2.0 units
5. **Add to combined buffer**: Calls `addMesh()` with the processed vertices/indices
6. Assigns material ID to the mesh

Returns the mesh ID (index into `meshes_` array), or -1 on failure.

## addMesh()

```cpp
int addMesh(const vector<Vertex>& vertices, const vector<uint32_t>& indices);
```

1. Records `vertexOffset` = current size of `allVertices_`, `indexOffset` = current size of `allIndices_`
2. Appends vertices and indices to the combined arrays
3. Creates a `MeshData` entry with offset/count and default material
4. Sets `buffersNeedRebuild_ = true`
5. Returns the new mesh ID

## rebuildGeometryBuffers()

Called at the start of `renderFrame()` when `buffersNeedRebuild_` is true:

1. `vkDeviceWaitIdle()` — ensure no in-flight frames reference the old buffers
2. Destroy existing vertex/index buffers
3. Create staging buffer (host-visible) → copy `allVertices_` data
4. Create device-local vertex buffer → `copyBuffer()` from staging
5. Destroy staging buffer
6. Repeat for index buffer with `allIndices_`
7. Clear `buffersNeedRebuild_` flag

This is a full rebuild — all geometry is re-uploaded. Fine for loading, but not suitable for per-frame mesh changes.

## Procedural Primitives

All primitives return a mesh ID and use `addMesh()` internally.

### Box (`createBoxMesh`)

- Parameters: width, height, length, RGB color
- 24 vertices (4 per face x 6 faces), 36 indices
- Each face has correct outward normal and UV mapping (0,0)-(1,1)

### Sphere (`createSphereMesh`)

- Parameters: radius, segments (longitude), rings (latitude), RGB color
- Generates `(rings + 1) x (segments + 1)` vertices using spherical coordinates
- UV: u = seg/segments, v = ring/rings
- Normals point radially outward

### Plane (`createPlaneMesh`)

- Parameters: width, height, RGB color
- 4 vertices, 6 indices (2 triangles)
- Lies in the XZ plane at Y=0, normal pointing up (0, 1, 0)
- UV maps (0,0)-(1,1)

### Cylinder (`createCylinderMesh`)

- Parameters: radius, height, segments, RGB color
- Side: 2 rings of `(segments + 1)` vertices with horizontal normals
- Top/bottom caps: center vertex + rim vertices with up/down normals
- Cap UVs use circular mapping: `(0.5 + cos*0.5, 0.5 + sin*0.5)`

### Capsule (`createCapsuleMesh`)

- Parameters: radius, height (body), segments, rings, RGB color
- Top hemisphere (offset up by halfH), cylinder body, bottom hemisphere (offset down by halfH)
- Rings must be even (split equally between hemispheres)
- Shared indexing across all sections for seamless normals

:::tip Where to Edit
**Adding a new primitive shape**: Create a new `createXxxMesh()` method in `renderer.h` and `renderer.cpp`. Generate vertices with positions, normals, colors, and UVs. Call `addMesh(verts, inds)` and return the result. Add the bridge function in `bridge.cpp` and `NativeBridge.cs`.

**Changing mesh loading behavior**: Modify `loadMesh()` in `renderer.cpp`. The auto-center/scale logic is at the end — remove or adjust the `2.0 / extent` scaling factor if you want original-size meshes.
:::

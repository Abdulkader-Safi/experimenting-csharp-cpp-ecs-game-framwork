# Frame Rendering

## renderFrame() Flow

Called once per frame from the C# game loop:

1. **Early out**: Skip if no entities exist
2. **Rebuild geometry**: If `buffersNeedRebuild_`, calls `rebuildGeometryBuffers()` (staging → device-local)
3. **Early out**: Skip if vertex/index buffers are null
4. **Wait for fence**: `vkWaitForFences(inFlightFences_[currentFrame_])` — blocks until previous frame's GPU work completes
5. **Acquire image**: `vkAcquireNextImageKHR` — gets next swapchain image index. If `OUT_OF_DATE`, recreates swapchain and returns
6. **Reset fence**: `vkResetFences` — unsignal the fence for this frame
7. **Update UBO**: `updateUniformBuffer(currentFrame_)` — uploads view/proj matrices and light data
8. **Build UI**: If debug overlay enabled, calls `buildDebugOverlayGeometry()`
9. **Reset + record command buffer**: `vkResetCommandBuffer` → `recordCommandBuffer`
10. **Submit**: `vkQueueSubmit` with wait on imageAvailable, signal renderFinished, signal fence
11. **Present**: `vkQueuePresentKHR` — if `OUT_OF_DATE` or `SUBOPTIMAL` or `framebufferResized_`, recreates swapchain
12. **Advance frame**: `currentFrame_ = (currentFrame_ + 1) % 2`

## recordCommandBuffer()

The command buffer records a single render pass:

```
vkCmdBeginRenderPass (clear color: 0.1, 0.1, 0.12, depth: 1.0)
  ├─ Bind 3D pipeline
  ├─ Set viewport + scissor
  ├─ Bind combined vertex buffer (offset 0)
  ├─ Bind combined index buffer (UINT32)
  ├─ Bind descriptor set 0 (UBO + lights)
  ├─ For each active entity:
  │   ├─ Bind descriptor set 1 (material texture)
  │   ├─ Push constants (model matrix)
  │   └─ vkCmdDrawIndexed(indexCount, 1, indexOffset, vertexOffset, 0)
  ├─ [If debug overlay enabled and has vertices]:
  │   └─ recordUICommands() (see UI Pipeline page)
  └─ vkCmdEndRenderPass
vkEndCommandBuffer
```

## updateUniformBuffer()

Uploads per-frame data to persistently-mapped UBOs:

```cpp
// View/projection
UniformBufferObject ubo{};
ubo.view = glm::lookAt(cameraEye_, cameraTarget_, cameraUp_);
ubo.proj = glm::perspective(glm::radians(cameraFov_), aspect, 0.1f, 100.0f);
ubo.proj[1][1] *= -1; // Vulkan Y-flip
memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));

// Lights
lightData_.cameraPos = glm::vec4(cameraEye_, 1.0f);
memcpy(lightBuffersMapped_[currentImage], &lightData_, sizeof(lightData_));
```

Near plane = 0.1, far plane = 100.0.

## Entity Management

### createEntity(meshId)

1. Validates mesh ID
2. Creates `EntityData` with identity transform, `active = true`
3. Reuses a free slot from `freeEntitySlots_` if available, otherwise appends
4. Returns entity ID

### setEntityTransform(entityId, float\* mat4x4)

Directly `memcpy`s 16 floats into the entity's transform matrix. Silently ignores invalid/inactive entity IDs.

### removeEntity(entityId)

Sets `active = false` and pushes the ID onto `freeEntitySlots_` for reuse. Does not compact the entity array.

### getActiveEntityCount()

Iterates all entities and counts active ones. Used by the debug overlay.

## Camera

`setCamera(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ, fovDegrees)` stores camera parameters. The actual view matrix is constructed in `updateUniformBuffer()` via `glm::lookAt()`.

:::tip Where to Edit
**Adding post-processing**: After `vkCmdEndRenderPass`, you could begin a second render pass targeting a different framebuffer. Alternatively, render the scene to an offscreen image and blit/compose in a second pass.

**Changing clear color**: Modify the `clearValues[0].color` in `recordCommandBuffer()` — currently `{0.1, 0.1, 0.12, 1.0}` (dark gray-blue).

**Modifying draw order**: Entities are drawn in array order. For transparency, you would need to sort entities by depth. For front-to-back (early-Z optimization), sort by distance from camera.
:::

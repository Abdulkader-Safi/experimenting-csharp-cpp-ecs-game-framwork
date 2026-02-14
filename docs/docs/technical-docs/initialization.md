# Init & Cleanup

## Initialization Sequence

The `init(int width, int height, const char* title)` method performs the full boot sequence:

1. `glfwInit()` -- initialize window system
2. `glfwCreateWindow()` -- create window with `GLFW_NO_API` (no OpenGL)
3. Set callbacks: `framebufferResizeCallback`, `scrollCallback`
4. `createInstance()` -- Vulkan instance with MoltenVK portability extensions
5. `createSurface()` -- window surface via GLFW
6. `pickPhysicalDevice()` -- select first suitable GPU
7. `createLogicalDevice()` -- create device + graphics/present queues
8. `createSwapchain()` -- prefer B8G8R8A8_SRGB + MAILBOX present mode
9. `createImageViews()` -- one image view per swapchain image
10. `createRenderPass()` -- color + depth attachments, single subpass
11. `createDescriptorSetLayout()` -- set 0: UBO (binding 0) + LightUBO (binding 1)
12. `createMaterialDescriptorSetLayout()` -- set 1: COMBINED_IMAGE_SAMPLER (binding 0)
13. `createGraphicsPipeline()` -- 3D pipeline: depth test, backface cull, no blend
14. `createCommandPool()` -- resettable command buffers
15. `createDepthResources()` -- depth image + view (D32_SFLOAT preferred)
16. `createFramebuffers()` -- one per swapchain image (color + depth)
17. `createUniformBuffers()` -- view/proj UBO + light UBO, per frame-in-flight (2), persistently mapped
18. `createDescriptorPool()` -- UBO descriptors
19. `createDescriptorSets()` -- bind UBO + light buffers to descriptor sets
20. `createTextureSampler()` -- LINEAR filter, REPEAT wrap
21. `createDefaultTexture()` -- 1x1 white RGBA pixel
22. `createMaterialDescriptorPool()` -- up to 64 material descriptors
23. `createMaterial(defaultTextureView_)` -- default material (id 0)
24. `createCommandBuffers()` -- one per frame-in-flight
25. `createSyncObjects()` -- 2 imageAvailable semaphores, 2 renderFinished semaphores, 2 fences (pre-signaled)
26. `createUIDescriptorSetLayout()` -- COMBINED_IMAGE_SAMPLER for font atlas
27. `createUIPipeline()` -- no depth, alpha blending, no culling
28. `createUIVertexBuffers()` -- host-visible, persistently mapped, 4096 max vertices per frame
29. `createFontResources()` -- load TTF via stb_truetype -> 512x512 R8_UNORM atlas (or 1x1 placeholder)
30. `createUIDescriptorPool()` + `createUIDescriptorSets()` -- bind font atlas

## Cleanup Order

`cleanup()` tears down in reverse dependency order:

1. `vkDeviceWaitIdle()` -- wait for all GPU work
2. `cleanupUIResources()` -- UI pipeline, font atlas, UI vertex buffers, UI descriptors
3. `cleanupMaterialResources()` -- material textures, sampler, descriptor pool, layout, default texture
4. `cleanupSwapchain()` -- depth resources, framebuffers, image views, swapchain
5. Uniform buffers + light buffers (per frame)
6. Sync objects (semaphores + fences)
7. Geometry buffers (vertex + index)
8. Descriptor pool + layout
9. Graphics pipeline + pipeline layout
10. Render pass
11. Command pool
12. Logical device
13. Surface
14. Instance
15. GLFW window + terminate

## Swapchain Recreation

On window resize (or `VK_ERROR_OUT_OF_DATE_KHR`):

1. Wait for zero-size framebuffer to resolve (minimized window)
2. `vkDeviceWaitIdle()`
3. `cleanupSwapchain()` -- destroy depth, framebuffers, image views, old swapchain
4. `createSwapchain()` -> `createImageViews()` -> `createDepthResources()` -> `createFramebuffers()`

:::tip Where to Edit
**Adding new GPU resources to init/cleanup**: Add the create call in `init()` after any resources it depends on. Add the matching destroy call in `cleanup()` before destroying its dependencies. If the resource depends on swapchain extent, also add recreate logic to `recreateSwapchain()`.
:::

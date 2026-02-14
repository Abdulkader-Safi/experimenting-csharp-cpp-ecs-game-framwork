# Vulkan Core Setup

## Instance Creation

`createInstance()` creates a `VkInstance` targeting Vulkan 1.0. Required extensions:

- GLFW surface extensions (queried via `glfwGetRequiredInstanceExtensions`)
- `VK_KHR_portability_enumeration` — required by MoltenVK on macOS
- `VK_KHR_get_physical_device_properties2` — dependency of portability

The instance creation flag includes `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`.

No validation layers are enabled by default.

## Physical Device Selection

`pickPhysicalDevice()` iterates all physical devices and picks the first that passes `isDeviceSuitable()`:

- Must have complete queue families (graphics + present)
- Must support `VK_KHR_swapchain`
- Must have at least one surface format and one present mode

The selected GPU name is printed to stdout.

## Logical Device & Queues

`createLogicalDevice()` creates the device with:

- Queue create infos for unique queue families (graphics may equal present)
- Device extensions: `VK_KHR_swapchain`, `VK_KHR_portability_subset`
- No special device features enabled

After creation, retrieves `graphicsQueue_` and `presentQueue_` handles.

## Swapchain

`createSwapchain()` configures:

- **Format**: Prefers `VK_FORMAT_B8G8R8A8_SRGB` + `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`, falls back to first available
- **Present mode**: Prefers `VK_PRESENT_MODE_MAILBOX_KHR` (triple buffering), falls back to `VK_PRESENT_MODE_FIFO_KHR` (vsync)
- **Extent**: Uses `currentExtent` if available, otherwise clamps framebuffer size to surface capabilities
- **Image count**: `minImageCount + 1`, capped by `maxImageCount`
- **Sharing mode**: `CONCURRENT` if graphics/present queues differ, `EXCLUSIVE` otherwise
- **Pre-transform**: current surface transform
- **Composite alpha**: opaque

## Render Pass

`createRenderPass()` defines a single subpass with two attachments:

| Attachment | Format                 | Load Op | Store Op  | Final Layout                     |
| ---------- | ---------------------- | ------- | --------- | -------------------------------- |
| Color (0)  | swapchain format       | CLEAR   | STORE     | PRESENT_SRC_KHR                  |
| Depth (1)  | D32_SFLOAT (preferred) | CLEAR   | DONT_CARE | DEPTH_STENCIL_ATTACHMENT_OPTIMAL |

The subpass dependency synchronizes `COLOR_ATTACHMENT_OUTPUT` and `EARLY_FRAGMENT_TESTS` stages from `VK_SUBPASS_EXTERNAL`.

## Depth Resources

`createDepthResources()` creates:

- A `VkImage` matching swapchain extent with optimal tiling
- Format preference order: `D32_SFLOAT` -> `D32_SFLOAT_S8_UINT` -> `D24_UNORM_S8_UINT`
- Device-local memory
- An image view with `DEPTH` aspect

## Command Pool & Buffers

- `createCommandPool()`: flags = `RESET_COMMAND_BUFFER_BIT` (allows per-frame reset), bound to graphics queue family
- `createCommandBuffers()`: allocates 2 primary command buffers (one per frame-in-flight)

## Sync Objects

`createSyncObjects()` creates per frame-in-flight (2 sets):

- `imageAvailableSemaphore` — signals when swapchain image is acquired
- `renderFinishedSemaphore` — signals when command buffer finishes
- `inFlightFence` — CPU-GPU sync, created **pre-signaled** so the first frame doesn't deadlock

The frame index cycles: `currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT`

:::tip Where to Edit
**Changing swapchain format**: Modify the preference in `createSwapchain()` where it checks `VK_FORMAT_B8G8R8A8_SRGB`. Also verify shader output format compatibility.

**Adding validation layers**: In `createInstance()`, add layer names to `createInfo.ppEnabledLayerNames` and set `createInfo.enabledLayerCount`. Common choice: `"VK_LAYER_KHRONOS_validation"`.

**Changing present mode**: Modify the preference loop in `createSwapchain()`. Use `FIFO` for guaranteed vsync, `MAILBOX` for low-latency, `IMMEDIATE` for uncapped FPS.
:::

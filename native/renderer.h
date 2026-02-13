#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

#define MAX_LIGHTS 8

enum LightType {
  LIGHT_DIRECTIONAL = 0,
  LIGHT_POINT = 1,
  LIGHT_SPOT = 2,
};

struct GpuLight {
  alignas(16) glm::vec4 position;  // xyz = pos, w = unused
  alignas(16) glm::vec4 direction; // xyz = dir, w = unused
  alignas(16) glm::vec4 color;     // xyz = RGB, w = intensity
  alignas(4) float innerCone;      // cos(inner angle) for spot
  alignas(4) float outerCone;      // cos(outer angle) for spot
  alignas(4) float radius;         // attenuation radius (0 = infinite)
  alignas(4) int type;             // 0=directional, 1=point, 2=spot
};

struct LightUBO {
  alignas(16) glm::vec4 cameraPos; // xyz = eye position
  alignas(4) int numLights;
  alignas(4) float ambientIntensity;
  alignas(8) float _pad[2];    // pad to 16 before array
  GpuLight lights[MAX_LIGHTS]; // 8 × 64 = 512
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 uv;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  static std::array<VkVertexInputAttributeDescription, 4>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attrs{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, normal);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[2].offset = offsetof(Vertex, color);

    attrs[3].binding = 0;
    attrs[3].location = 3;
    attrs[3].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[3].offset = offsetof(Vertex, uv);

    return attrs;
  }
};

struct UIVertex {
  glm::vec2 pos;
  glm::vec2 uv;
  glm::vec4 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(UIVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attrs{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = offsetof(UIVertex, pos);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = offsetof(UIVertex, uv);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[2].offset = offsetof(UIVertex, color);

    return attrs;
  }
};

struct GlyphInfo {
  float x0, y0, x1, y1; // UV coordinates in atlas
  float xoff, yoff, xadvance;
  float width, height; // pixel dimensions
};

struct UIPushConstants {
  glm::vec2 screenSize;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct PushConstantData {
  glm::mat4 model;
};

struct MaterialData {
  VkImage textureImage = VK_NULL_HANDLE;
  VkDeviceMemory textureMemory = VK_NULL_HANDLE;
  VkImageView textureView = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  bool ownsTexture = true;
};

struct MeshData {
  int32_t vertexOffset;
  uint32_t indexOffset;
  uint32_t indexCount;
  int materialId = 0;
};

struct EntityData {
  int meshId;
  glm::mat4 transform;
  bool active;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

class VulkanRenderer {
public:
  bool init(int width, int height, const char *title);
  void cleanup();
  bool shouldClose() const;
  void pollEvents();
  int isKeyPressed(int glfwKey) const;
  void renderFrame();

  // Legacy API (backward compat)
  bool loadModel(const char *path);
  void setRotation(float rx, float ry, float rz);

  // Multi-entity API
  int loadMesh(const char *path);
  int createEntity(int meshId);

  // Procedural primitives
  int createBoxMesh(float width, float height, float length, float r, float g,
                    float b);
  int createSphereMesh(float radius, int segments, int rings, float r, float g,
                       float b);
  int createPlaneMesh(float width, float height, float r, float g, float b);
  int createCylinderMesh(float radius, float height, int segments, float r,
                         float g, float b);
  int createCapsuleMesh(float radius, float height, int segments, int rings,
                        float r, float g, float b);
  void setEntityTransform(int entityId, const float *mat4x4);
  void removeEntity(int entityId);

  // Camera
  void setCamera(float eyeX, float eyeY, float eyeZ, float targetX,
                 float targetY, float targetZ, float upX, float upY, float upZ,
                 float fovDegrees);

  // Cursor
  void getCursorPos(double &x, double &y) const;
  void setCursorLocked(bool locked);
  bool isCursorLocked() const;

  // Mouse buttons & scroll
  int isMouseButtonPressed(int button) const;
  void getScrollOffset(float &x, float &y) const;
  void resetScrollOffset();

  // Lighting
  void setLight(int index, int type, float posX, float posY, float posZ,
                float dirX, float dirY, float dirZ, float r, float g, float b,
                float intensity, float radius, float innerCone,
                float outerCone);
  void clearLight(int index);
  void setAmbientIntensity(float intensity);

  // Time
  void updateTime();
  float getDeltaTime() const;
  float getTotalTime() const;

  // Debug overlay
  void setDebugOverlay(bool enabled);
  int getActiveEntityCount() const;

  // Debug wireframe entities (rendered only when debug overlay is on)
  int createDebugEntity(int meshId);
  void setDebugEntityTransform(int entityId, const float *mat4x4);
  void removeDebugEntity(int entityId);
  void clearDebugEntities();

private:
  // Window
  GLFWwindow *window_ = nullptr;
  int width_ = 800;
  int height_ = 600;
  bool framebufferResized_ = false;

  // Vulkan core
  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  VkQueue presentQueue_ = VK_NULL_HANDLE;
  QueueFamilyIndices queueFamilies_;

  // Swapchain
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat swapchainFormat_;
  VkExtent2D swapchainExtent_;
  std::vector<VkImage> swapchainImages_;
  std::vector<VkImageView> swapchainImageViews_;
  std::vector<VkFramebuffer> swapchainFramebuffers_;

  // Depth
  VkImage depthImage_ = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
  VkImageView depthImageView_ = VK_NULL_HANDLE;

  // Pipeline
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;

  // Buffers
  VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
  VkBuffer indexBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;

  // Uniform buffers (per frame in flight)
  static const int MAX_FRAMES_IN_FLIGHT = 2;
  std::vector<VkBuffer> uniformBuffers_;
  std::vector<VkDeviceMemory> uniformBuffersMemory_;
  std::vector<void *> uniformBuffersMapped_;

  // Light uniform buffers (per frame in flight)
  std::vector<VkBuffer> lightBuffers_;
  std::vector<VkDeviceMemory> lightBuffersMemory_;
  std::vector<void *> lightBuffersMapped_;
  LightUBO lightData_{};

  // Descriptors
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets_;

  // Commands
  VkCommandPool commandPool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers_;

  // Sync
  std::vector<VkSemaphore> imageAvailableSemaphores_;
  std::vector<VkSemaphore> renderFinishedSemaphores_;
  std::vector<VkFence> inFlightFences_;
  uint32_t currentFrame_ = 0;

  // Multi-mesh geometry (combined buffer)
  std::vector<MeshData> meshes_;
  std::vector<Vertex> allVertices_;
  std::vector<uint32_t> allIndices_;
  bool buffersNeedRebuild_ = false;

  // Entities
  std::vector<EntityData> entities_;
  std::vector<int> freeEntitySlots_;

  // Debug wireframe pipeline + entities
  VkPipeline debugPipeline_ = VK_NULL_HANDLE;
  std::vector<EntityData> debugEntities_;
  std::vector<int> freeDebugEntitySlots_;

  // Legacy compat
  float rotX_ = 0.0f, rotY_ = 0.0f, rotZ_ = 0.0f;
  int legacyMeshId_ = -1;
  int legacyEntityId_ = -1;

  // Camera state
  glm::vec3 cameraEye_ = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 cameraTarget_ = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraUp_ = glm::vec3(0.0f, 1.0f, 0.0f);
  float cameraFov_ = 45.0f;
  bool cursorLocked_ = false;

  // Scroll accumulator
  float scrollOffsetX_ = 0.0f;
  float scrollOffsetY_ = 0.0f;

  // Time
  double lastFrameTime_ = 0.0;
  float deltaTime_ = 0.016f;
  float totalTime_ = 0.0f;

  // Init helpers
  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createDepthResources();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createCommandBuffers();
  void createSyncObjects();

  // Swapchain recreation
  void recreateSwapchain();
  void cleanupSwapchain();

  // Multi-entity
  void rebuildGeometryBuffers();
  int addMesh(const std::vector<Vertex> &vertices,
              const std::vector<uint32_t> &indices);

  // One-time command buffer helpers
  VkCommandBuffer beginOneTimeCommands();
  void endOneTimeCommands(VkCommandBuffer commandBuffer);

  // Entity pool helpers
  int allocateEntity(std::vector<EntityData> &pool, std::vector<int> &freeSlots,
                     int meshId);
  void recalcNumLights();

  // Helpers
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
  bool isDeviceSuitable(VkPhysicalDevice device) const;
  VkShaderModule createShaderModule(const std::vector<char> &code) const;
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) const;
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &memory) const;
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void createImage(uint32_t w, uint32_t h, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &memory) const;
  VkImageView createImageView(VkImage image, VkFormat format,
                              VkImageAspectFlags aspectFlags) const;
  VkFormat findDepthFormat() const;
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void updateUniformBuffer(uint32_t currentImage);

  // UI pipeline
  VkPipeline uiPipeline_ = VK_NULL_HANDLE;
  VkPipelineLayout uiPipelineLayout_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout uiDescriptorSetLayout_ = VK_NULL_HANDLE;
  VkDescriptorPool uiDescriptorPool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> uiDescriptorSets_;

  // Font atlas
  VkImage fontImage_ = VK_NULL_HANDLE;
  VkDeviceMemory fontImageMemory_ = VK_NULL_HANDLE;
  VkImageView fontImageView_ = VK_NULL_HANDLE;
  VkSampler fontSampler_ = VK_NULL_HANDLE;
  bool fontLoaded_ = false;

  // UI vertex buffers (per frame, host-visible, persistently mapped)
  static const int UI_MAX_VERTICES = 4096;
  std::vector<VkBuffer> uiVertexBuffers_;
  std::vector<VkDeviceMemory> uiVertexBuffersMemory_;
  std::vector<void *> uiVertexBuffersMapped_;
  uint32_t uiVertexCount_ = 0;
  std::vector<UIVertex> uiVertices_;

  // Debug overlay state
  bool debugOverlayEnabled_ = false;
  float smoothedFps_ = 60.0f;

  // Glyph data
  static const int FONT_ATLAS_SIZE = 512;
  static const int GLYPH_FIRST = 32;
  static const int GLYPH_COUNT = 95; // ASCII 32-126
  GlyphInfo glyphs_[95];
  float fontPixelHeight_ = 20.0f;

  // UI init helpers
  void createUIPipeline();
  void createUIDescriptorSetLayout();
  void createUIDescriptorPool();
  void createUIDescriptorSets();
  void createUIVertexBuffers();
  void createFontResources();
  void cleanupUIResources();

  // UI rendering
  void recordUICommands(VkCommandBuffer commandBuffer);
  void buildDebugOverlayGeometry();
  void appendText(const char *text, float x, float y, glm::vec4 color);
  void appendQuad(float x, float y, float w, float h, glm::vec4 color);

  // Image layout transition helper
  void transitionImageLayout(VkImage image, VkImageLayout oldLayout,
                             VkImageLayout newLayout);

  // Material/texture system
  static const int MAX_MATERIALS = 64;
  VkDescriptorSetLayout materialDescriptorSetLayout_ = VK_NULL_HANDLE;
  VkDescriptorPool materialDescriptorPool_ = VK_NULL_HANDLE;
  VkSampler textureSampler_ = VK_NULL_HANDLE;
  std::vector<MaterialData> materials_;
  VkImage defaultTextureImage_ = VK_NULL_HANDLE;
  VkDeviceMemory defaultTextureMemory_ = VK_NULL_HANDLE;
  VkImageView defaultTextureView_ = VK_NULL_HANDLE;
  int defaultMaterialId_ = 0;

  void createMaterialDescriptorSetLayout();
  void createMaterialDescriptorPool();
  void createTextureSampler();
  void createDefaultTexture();
  int createMaterial(VkImageView textureView);
  void cleanupMaterialResources();
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  int loadTextureFromMemory(const uint8_t *data, size_t size);

  static void framebufferResizeCallback(GLFWwindow *window, int w, int h);
  static void scrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset);
  static std::vector<char> readFile(const std::string &filename);
};

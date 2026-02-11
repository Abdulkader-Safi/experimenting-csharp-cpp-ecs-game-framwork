#define CGLTF_IMPLEMENTATION
#include "vendor/cgltf.h"
#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static void checkVk(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(msg) + " (VkResult " + std::to_string(result) + ")");
    }
}

void VulkanRenderer::framebufferResizeCallback(GLFWwindow* window, int /*w*/, int /*h*/) {
    auto* app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
}

void VulkanRenderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    app->scrollOffsetX_ += static_cast<float>(xoffset);
    app->scrollOffsetY_ += static_cast<float>(yoffset);
}

std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool VulkanRenderer::init(int width, int height, const char* title) {
    width_ = width;
    height_ = height;

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetScrollCallback(window_, scrollCallback);

    try {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    } catch (const std::exception& e) {
        std::cerr << "Vulkan init failed: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void VulkanRenderer::cleanup() {
    if (device_) vkDeviceWaitIdle(device_);

    cleanupSwapchain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (uniformBuffers_.size() > i) {
            vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
            vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
        }
        if (lightBuffers_.size() > i) {
            vkDestroyBuffer(device_, lightBuffers_[i], nullptr);
            vkFreeMemory(device_, lightBuffersMemory_[i], nullptr);
        }
        if (imageAvailableSemaphores_.size() > i)
            vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
        if (renderFinishedSemaphores_.size() > i)
            vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
        if (inFlightFences_.size() > i)
            vkDestroyFence(device_, inFlightFences_[i], nullptr);
    }

    if (indexBuffer_) vkDestroyBuffer(device_, indexBuffer_, nullptr);
    if (indexBufferMemory_) vkFreeMemory(device_, indexBufferMemory_, nullptr);
    if (vertexBuffer_) vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    if (vertexBufferMemory_) vkFreeMemory(device_, vertexBufferMemory_, nullptr);

    if (descriptorPool_) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    if (descriptorSetLayout_) vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
    if (graphicsPipeline_) vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    if (pipelineLayout_) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    if (renderPass_) vkDestroyRenderPass(device_, renderPass_, nullptr);
    if (commandPool_) vkDestroyCommandPool(device_, commandPool_, nullptr);
    if (device_) vkDestroyDevice(device_, nullptr);
    if (surface_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (instance_) vkDestroyInstance(instance_, nullptr);

    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Multi-entity API
// ---------------------------------------------------------------------------

int VulkanRenderer::loadMesh(const char* path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success) {
        std::cerr << "Failed to parse glTF: " << path << std::endl;
        return -1;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        std::cerr << "Failed to load glTF buffers" << std::endl;
        cgltf_free(data);
        return -1;
    }

    // Temporary buffers for this mesh (0-based indices)
    std::vector<Vertex> meshVertices;
    std::vector<uint32_t> meshIndices;

    for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
        cgltf_mesh& mesh = data->meshes[mi];
        for (cgltf_size pi = 0; pi < mesh.primitives_count; pi++) {
            cgltf_primitive& prim = mesh.primitives[pi];
            if (prim.type != cgltf_primitive_type_triangles)
                continue;

            uint32_t vertexOffset = static_cast<uint32_t>(meshVertices.size());

            cgltf_accessor* posAccessor = nullptr;
            cgltf_accessor* normAccessor = nullptr;
            for (cgltf_size ai = 0; ai < prim.attributes_count; ai++) {
                if (prim.attributes[ai].type == cgltf_attribute_type_position)
                    posAccessor = prim.attributes[ai].data;
                else if (prim.attributes[ai].type == cgltf_attribute_type_normal)
                    normAccessor = prim.attributes[ai].data;
            }

            if (!posAccessor) continue;

            glm::vec3 baseColor(0.7f, 0.7f, 0.8f);
            if (prim.material && prim.material->has_pbr_metallic_roughness) {
                float* c = prim.material->pbr_metallic_roughness.base_color_factor;
                baseColor = glm::vec3(c[0], c[1], c[2]);
            }

            for (cgltf_size vi = 0; vi < posAccessor->count; vi++) {
                Vertex v{};
                cgltf_accessor_read_float(posAccessor, vi, &v.pos.x, 3);
                if (normAccessor)
                    cgltf_accessor_read_float(normAccessor, vi, &v.normal.x, 3);
                else
                    v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                v.color = baseColor;
                meshVertices.push_back(v);
            }

            if (prim.indices) {
                for (cgltf_size ii = 0; ii < prim.indices->count; ii++) {
                    uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii));
                    meshIndices.push_back(vertexOffset + idx);
                }
            } else {
                for (uint32_t vi = 0; vi < static_cast<uint32_t>(posAccessor->count); vi++)
                    meshIndices.push_back(vertexOffset + vi);
            }
        }
    }

    cgltf_free(data);

    if (meshVertices.empty()) {
        std::cerr << "No geometry found in model" << std::endl;
        return -1;
    }

    // Auto-center and scale to fit
    glm::vec3 minB(std::numeric_limits<float>::max());
    glm::vec3 maxB(std::numeric_limits<float>::lowest());
    for (auto& v : meshVertices) {
        minB = glm::min(minB, v.pos);
        maxB = glm::max(maxB, v.pos);
    }
    glm::vec3 center = (minB + maxB) * 0.5f;
    float extent = glm::length(maxB - minB);
    float scale = (extent > 0.0f) ? 2.0f / extent : 1.0f;
    for (auto& v : meshVertices) {
        v.pos = (v.pos - center) * scale;
    }

    return addMesh(meshVertices, meshIndices);
}

int VulkanRenderer::addMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    if (vertices.empty() || indices.empty()) {
        std::cerr << "Cannot add empty mesh" << std::endl;
        return -1;
    }

    MeshData md{};
    md.vertexOffset = static_cast<int32_t>(allVertices_.size());
    md.indexOffset = static_cast<uint32_t>(allIndices_.size());
    md.indexCount = static_cast<uint32_t>(indices.size());

    allVertices_.insert(allVertices_.end(), vertices.begin(), vertices.end());
    allIndices_.insert(allIndices_.end(), indices.begin(), indices.end());

    int meshId = static_cast<int>(meshes_.size());
    meshes_.push_back(md);
    buffersNeedRebuild_ = true;

    std::cout << "Added mesh " << meshId << ": " << vertices.size() << " vertices, "
              << indices.size() << " indices" << std::endl;
    return meshId;
}

// ---------------------------------------------------------------------------
// Procedural primitives
// ---------------------------------------------------------------------------

int VulkanRenderer::createBoxMesh(float width, float height, float length, float r, float g, float b) {
    float hw = width * 0.5f, hh = height * 0.5f, hl = length * 0.5f;
    glm::vec3 color(r, g, b);

    std::vector<Vertex> verts;
    std::vector<uint32_t> inds;
    verts.reserve(24);
    inds.reserve(36);

    // Helper: add a face (4 verts + 6 indices)
    auto addFace = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 n) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        verts.push_back({p0, n, color});
        verts.push_back({p1, n, color});
        verts.push_back({p2, n, color});
        verts.push_back({p3, n, color});
        inds.push_back(base + 0); inds.push_back(base + 1); inds.push_back(base + 2);
        inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 3);
    };

    // +Z face
    addFace({-hw, -hh,  hl}, { hw, -hh,  hl}, { hw,  hh,  hl}, {-hw,  hh,  hl}, { 0, 0, 1});
    // -Z face
    addFace({ hw, -hh, -hl}, {-hw, -hh, -hl}, {-hw,  hh, -hl}, { hw,  hh, -hl}, { 0, 0,-1});
    // +Y face
    addFace({-hw,  hh,  hl}, { hw,  hh,  hl}, { hw,  hh, -hl}, {-hw,  hh, -hl}, { 0, 1, 0});
    // -Y face
    addFace({-hw, -hh, -hl}, { hw, -hh, -hl}, { hw, -hh,  hl}, {-hw, -hh,  hl}, { 0,-1, 0});
    // +X face
    addFace({ hw, -hh,  hl}, { hw, -hh, -hl}, { hw,  hh, -hl}, { hw,  hh,  hl}, { 1, 0, 0});
    // -X face
    addFace({-hw, -hh, -hl}, {-hw, -hh,  hl}, {-hw,  hh,  hl}, {-hw,  hh, -hl}, {-1, 0, 0});

    return addMesh(verts, inds);
}

int VulkanRenderer::createSphereMesh(float radius, int segments, int rings, float r, float g, float b) {
    glm::vec3 color(r, g, b);
    std::vector<Vertex> verts;
    std::vector<uint32_t> inds;

    for (int ring = 0; ring <= rings; ring++) {
        float phi = static_cast<float>(M_PI) * static_cast<float>(ring) / static_cast<float>(rings);
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int seg = 0; seg <= segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            glm::vec3 normal(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);
            glm::vec3 pos = normal * radius;
            verts.push_back({pos, normal, color});
        }
    }

    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            uint32_t curr = static_cast<uint32_t>(ring * (segments + 1) + seg);
            uint32_t next = curr + static_cast<uint32_t>(segments + 1);

            inds.push_back(curr);
            inds.push_back(curr + 1);
            inds.push_back(next);

            inds.push_back(curr + 1);
            inds.push_back(next + 1);
            inds.push_back(next);
        }
    }

    return addMesh(verts, inds);
}

int VulkanRenderer::createPlaneMesh(float width, float height, float r, float g, float b) {
    float hw = width * 0.5f, hh = height * 0.5f;
    glm::vec3 color(r, g, b);
    glm::vec3 normal(0.0f, 1.0f, 0.0f);

    std::vector<Vertex> verts = {
        {{-hw, 0.0f,  hh}, normal, color},
        {{ hw, 0.0f,  hh}, normal, color},
        {{ hw, 0.0f, -hh}, normal, color},
        {{-hw, 0.0f, -hh}, normal, color},
    };
    std::vector<uint32_t> inds = { 0, 1, 2, 0, 2, 3 };

    return addMesh(verts, inds);
}

int VulkanRenderer::createCylinderMesh(float radius, float height, int segments, float r, float g, float b) {
    glm::vec3 color(r, g, b);
    float halfH = height * 0.5f;

    std::vector<Vertex> verts;
    std::vector<uint32_t> inds;

    // Side vertices: 2 rings of (segments+1) vertices
    for (int seg = 0; seg <= segments; seg++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
        float cosT = cosf(theta);
        float sinT = sinf(theta);
        glm::vec3 normal(cosT, 0.0f, sinT);

        // Bottom ring
        verts.push_back({{radius * cosT, -halfH, radius * sinT}, normal, color});
        // Top ring
        verts.push_back({{radius * cosT,  halfH, radius * sinT}, normal, color});
    }

    // Side indices
    for (int seg = 0; seg < segments; seg++) {
        uint32_t bl = static_cast<uint32_t>(seg * 2);
        uint32_t tl = bl + 1;
        uint32_t br = bl + 2;
        uint32_t tr = bl + 3;

        inds.push_back(bl); inds.push_back(tl); inds.push_back(br);
        inds.push_back(tl); inds.push_back(tr); inds.push_back(br);
    }

    // Top cap
    uint32_t topCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0.0f, halfH, 0.0f}, {0.0f, 1.0f, 0.0f}, color});
    uint32_t topRimStart = static_cast<uint32_t>(verts.size());
    for (int seg = 0; seg < segments; seg++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
        verts.push_back({{radius * cosf(theta), halfH, radius * sinf(theta)}, {0.0f, 1.0f, 0.0f}, color});
    }
    for (int seg = 0; seg < segments; seg++) {
        uint32_t next = (seg + 1) % segments;
        inds.push_back(topCenter);
        inds.push_back(topRimStart + static_cast<uint32_t>(next));
        inds.push_back(topRimStart + static_cast<uint32_t>(seg));
    }

    // Bottom cap
    uint32_t botCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, color});
    uint32_t botRimStart = static_cast<uint32_t>(verts.size());
    for (int seg = 0; seg < segments; seg++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
        verts.push_back({{radius * cosf(theta), -halfH, radius * sinf(theta)}, {0.0f, -1.0f, 0.0f}, color});
    }
    for (int seg = 0; seg < segments; seg++) {
        uint32_t next = (seg + 1) % segments;
        inds.push_back(botCenter);
        inds.push_back(botRimStart + static_cast<uint32_t>(seg));
        inds.push_back(botRimStart + static_cast<uint32_t>(next));
    }

    return addMesh(verts, inds);
}

int VulkanRenderer::createCapsuleMesh(float radius, float height, int segments, int rings, float r, float g, float b) {
    glm::vec3 color(r, g, b);
    float halfH = height * 0.5f;
    // rings must be even for hemisphere split
    int halfRings = rings / 2;

    std::vector<Vertex> verts;
    std::vector<uint32_t> inds;

    // Top hemisphere (rings 0..halfRings), offset up by halfH
    for (int ring = 0; ring <= halfRings; ring++) {
        float phi = static_cast<float>(M_PI) * 0.5f * static_cast<float>(ring) / static_cast<float>(halfRings);
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int seg = 0; seg <= segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            glm::vec3 normal(sinPhi * cosf(theta), cosPhi, sinPhi * sinf(theta));
            glm::vec3 pos = normal * radius + glm::vec3(0.0f, halfH, 0.0f);
            verts.push_back({pos, normal, color});
        }
    }

    // Cylinder body: 2 extra rings at the equator connecting top and bottom hemispheres
    // Top equator ring is already the last ring of the top hemisphere (ring == halfRings)
    // We add the bottom equator ring
    {
        for (int seg = 0; seg <= segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            float cosT = cosf(theta);
            float sinT = sinf(theta);
            glm::vec3 normal(cosT, 0.0f, sinT);
            glm::vec3 pos(radius * cosT, -halfH, radius * sinT);
            verts.push_back({pos, normal, color});
        }
    }

    // Bottom hemisphere (rings 0..halfRings), offset down by halfH
    // ring 0 = equator (already added), so start from ring 1
    for (int ring = 1; ring <= halfRings; ring++) {
        float phi = static_cast<float>(M_PI) * 0.5f + static_cast<float>(M_PI) * 0.5f * static_cast<float>(ring) / static_cast<float>(halfRings);
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int seg = 0; seg <= segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            glm::vec3 normal(sinPhi * cosf(theta), cosPhi, sinPhi * sinf(theta));
            glm::vec3 pos = normal * radius + glm::vec3(0.0f, -halfH, 0.0f);
            verts.push_back({pos, normal, color});
        }
    }

    // Index generation
    // Total ring rows: halfRings (top hemi) + 1 (cylinder body) + halfRings (bottom hemi)
    int totalRows = halfRings + 1 + halfRings;
    for (int row = 0; row < totalRows; row++) {
        for (int seg = 0; seg < segments; seg++) {
            uint32_t curr = static_cast<uint32_t>(row * (segments + 1) + seg);
            uint32_t next = curr + static_cast<uint32_t>(segments + 1);

            inds.push_back(curr);
            inds.push_back(curr + 1);
            inds.push_back(next);

            inds.push_back(curr + 1);
            inds.push_back(next + 1);
            inds.push_back(next);
        }
    }

    return addMesh(verts, inds);
}

int VulkanRenderer::createEntity(int meshId) {
    if (meshId < 0 || meshId >= static_cast<int>(meshes_.size())) {
        std::cerr << "Invalid mesh ID: " << meshId << std::endl;
        return -1;
    }

    EntityData ent{};
    ent.meshId = meshId;
    ent.transform = glm::mat4(1.0f);
    ent.active = true;

    int entityId;
    if (!freeEntitySlots_.empty()) {
        entityId = freeEntitySlots_.back();
        freeEntitySlots_.pop_back();
        entities_[entityId] = ent;
    } else {
        entityId = static_cast<int>(entities_.size());
        entities_.push_back(ent);
    }

    return entityId;
}

void VulkanRenderer::setEntityTransform(int entityId, const float* mat4x4) {
    if (entityId < 0 || entityId >= static_cast<int>(entities_.size()) || !entities_[entityId].active) {
        return;
    }
    memcpy(&entities_[entityId].transform, mat4x4, sizeof(float) * 16);
}

void VulkanRenderer::removeEntity(int entityId) {
    if (entityId < 0 || entityId >= static_cast<int>(entities_.size())) return;
    entities_[entityId].active = false;
    freeEntitySlots_.push_back(entityId);
}

void VulkanRenderer::rebuildGeometryBuffers() {
    if (allVertices_.empty() || allIndices_.empty()) return;

    vkDeviceWaitIdle(device_);

    if (vertexBuffer_) { vkDestroyBuffer(device_, vertexBuffer_, nullptr); vertexBuffer_ = VK_NULL_HANDLE; }
    if (vertexBufferMemory_) { vkFreeMemory(device_, vertexBufferMemory_, nullptr); vertexBufferMemory_ = VK_NULL_HANDLE; }
    if (indexBuffer_) { vkDestroyBuffer(device_, indexBuffer_, nullptr); indexBuffer_ = VK_NULL_HANDLE; }
    if (indexBufferMemory_) { vkFreeMemory(device_, indexBufferMemory_, nullptr); indexBufferMemory_ = VK_NULL_HANDLE; }

    // Build vertex buffer from allVertices_
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * allVertices_.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, allVertices_.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer_, vertexBufferMemory_);
        copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);
    }

    // Build index buffer from allIndices_
    {
        VkDeviceSize bufferSize = sizeof(uint32_t) * allIndices_.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, allIndices_.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer_, indexBufferMemory_);
        copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);
    }

    buffersNeedRebuild_ = false;
}

// ---------------------------------------------------------------------------
// Legacy API (backward compat shims)
// ---------------------------------------------------------------------------

bool VulkanRenderer::loadModel(const char* path) {
    int meshId = loadMesh(path);
    if (meshId < 0) return false;

    legacyMeshId_ = meshId;
    legacyEntityId_ = createEntity(meshId);
    return legacyEntityId_ >= 0;
}

void VulkanRenderer::setRotation(float rx, float ry, float rz) {
    rotX_ = rx;
    rotY_ = ry;
    rotZ_ = rz;

    if (legacyEntityId_ >= 0) {
        glm::mat4 model(1.0f);
        model = glm::rotate(model, glm::radians(rotX_), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotY_), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotZ_), glm::vec3(0.0f, 0.0f, 1.0f));
        setEntityTransform(legacyEntityId_, &model[0][0]);
    }
}

bool VulkanRenderer::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void VulkanRenderer::pollEvents() {
    glfwPollEvents();
}

int VulkanRenderer::isKeyPressed(int glfwKey) const {
    return glfwGetKey(window_, glfwKey) == GLFW_PRESS ? 1 : 0;
}

void VulkanRenderer::renderFrame() {
    if (entities_.empty()) return;

    // Rebuild geometry buffers if meshes were added
    if (buffersNeedRebuild_) {
        rebuildGeometryBuffers();
    }

    if (!vertexBuffer_ || !indexBuffer_) return;

    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                             imageAvailableSemaphores_[currentFrame_],
                                             VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    updateUniformBuffer(currentFrame_);

    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    checkVk(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]),
            "Failed to submit draw command buffer");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { swapchain_ };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ---------------------------------------------------------------------------
// Vulkan setup
// ---------------------------------------------------------------------------

void VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "glTF Viewer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    if (!glfwExts || glfwExtCount == 0)
        throw std::runtime_error(
            "GLFW cannot find Vulkan loader. "
            "Ensure DYLD_LIBRARY_PATH includes /opt/homebrew/lib");
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back("VK_KHR_get_physical_device_properties2");

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    checkVk(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create Vulkan instance");
}

void VulkanRenderer::createSurface() {
    checkVk(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_),
            "Failed to create window surface");
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;

        if (indices.isComplete()) break;
    }
    return indices;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) const {
    QueueFamilyIndices indices = findQueueFamilies(device);
    if (!indices.isComplete()) return false;

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());

    bool swapchainSupported = false;
    for (auto& ext : available) {
        if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            swapchainSupported = true;
            break;
        }
    }
    if (!swapchainSupported) return false;

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

    return formatCount > 0 && presentModeCount > 0;
}

void VulkanRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("No GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice_ = device;
            break;
        }
    }
    if (physicalDevice_ == VK_NULL_HANDLE)
        throw std::runtime_error("No suitable GPU found");

    queueFamilies_ = findQueueFamilies(physicalDevice_);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice_, &props);
    std::cout << "GPU: " << props.deviceName << std::endl;
}

void VulkanRenderer::createLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies_.graphicsFamily.value(),
        queueFamilies_.presentFamily.value()
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    for (uint32_t family : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = family;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    checkVk(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_),
            "Failed to create logical device");

    vkGetDeviceQueue(device_, queueFamilies_.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.presentFamily.value(), 0, &presentQueue_);
}

void VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto& pm : presentModes) {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = pm;
            break;
        }
    }

    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        extent.width = std::clamp(static_cast<uint32_t>(w),
                                   capabilities.minImageExtent.width,
                                   capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(h),
                                    capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        queueFamilies_.graphicsFamily.value(),
        queueFamilies_.presentFamily.value()
    };

    if (queueFamilies_.graphicsFamily != queueFamilies_.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    checkVk(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_),
            "Failed to create swap chain");

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    swapchainFormat_ = surfaceFormat.format;
    swapchainExtent_ = extent;
}

void VulkanRenderer::createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        swapchainImageViews_[i] = createImageView(swapchainImages_[i], swapchainFormat_,
                                                    VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    checkVk(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_),
            "Failed to create render pass");
}

void VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding lightBinding{};
    lightBinding.binding = 1;
    lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightBinding.descriptorCount = 1;
    lightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, lightBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    checkVk(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_),
            "Failed to create descriptor set layout");
}

void VulkanRenderer::createGraphicsPipeline() {
    auto vertShaderCode = readFile("build/shaders/vert.spv");
    auto fragShaderCode = readFile("build/shaders/frag.spv");

    VkShaderModule vertModule = createShaderModule(vertShaderCode);
    VkShaderModule fragModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Push constant range for per-entity model matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    checkVk(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_),
            "Failed to create pipeline layout");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    checkVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                       &graphicsPipeline_),
            "Failed to create graphics pipeline");

    vkDestroyShaderModule(device_, fragModule, nullptr);
    vkDestroyShaderModule(device_, vertModule, nullptr);
}

void VulkanRenderer::createFramebuffers() {
    swapchainFramebuffers_.resize(swapchainImageViews_.size());
    for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews_[i],
            depthImageView_
        };

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass_;
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = swapchainExtent_.width;
        fbInfo.height = swapchainExtent_.height;
        fbInfo.layers = 1;

        checkVk(vkCreateFramebuffer(device_, &fbInfo, nullptr, &swapchainFramebuffers_[i]),
                "Failed to create framebuffer");
    }
}

void VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilies_.graphicsFamily.value();

    checkVk(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_),
            "Failed to create command pool");
}

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(swapchainExtent_.width, swapchainExtent_.height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage_, depthImageMemory_);
    depthImageView_ = createImageView(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * allVertices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, allVertices_.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vertexBuffer_, vertexBufferMemory_);

    copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(uint32_t) * allIndices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, allIndices_.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 indexBuffer_, indexBufferMemory_);

    copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers_[i], uniformBuffersMemory_[i]);
        vkMapMemory(device_, uniformBuffersMemory_[i], 0, bufferSize, 0,
                     &uniformBuffersMapped_[i]);
    }

    // Light UBO buffers
    VkDeviceSize lightSize = sizeof(LightUBO);

    lightBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    lightBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
    lightBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(lightSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     lightBuffers_[i], lightBuffersMemory_[i]);
        vkMapMemory(device_, lightBuffersMemory_[i], 0, lightSize, 0,
                     &lightBuffersMapped_[i]);
    }

    lightData_.ambientIntensity = 0.15f;
}

void VulkanRenderer::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    checkVk(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_),
            "Failed to create descriptor pool");
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    checkVk(vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()),
            "Failed to allocate descriptor sets");

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo uboInfo{};
        uboInfo.buffer = uniformBuffers_[i];
        uboInfo.offset = 0;
        uboInfo.range = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = lightBuffers_[i];
        lightInfo.offset = 0;
        lightInfo.range = sizeof(LightUBO);

        std::array<VkWriteDescriptorSet, 2> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets_[i];
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &uboInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets_[i];
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &lightInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()),
                               writes.data(), 0, nullptr);
    }
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    checkVk(vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()),
            "Failed to allocate command buffers");
}

void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        checkVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]),
                "Failed to create semaphore");
        checkVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]),
                "Failed to create semaphore");
        checkVk(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]),
                "Failed to create fence");
    }
}

// ---------------------------------------------------------------------------
// Swapchain recreation
// ---------------------------------------------------------------------------

void VulkanRenderer::cleanupSwapchain() {
    if (depthImageView_) vkDestroyImageView(device_, depthImageView_, nullptr);
    depthImageView_ = VK_NULL_HANDLE;
    if (depthImage_) vkDestroyImage(device_, depthImage_, nullptr);
    depthImage_ = VK_NULL_HANDLE;
    if (depthImageMemory_) vkFreeMemory(device_, depthImageMemory_, nullptr);
    depthImageMemory_ = VK_NULL_HANDLE;

    for (auto fb : swapchainFramebuffers_)
        vkDestroyFramebuffer(device_, fb, nullptr);
    swapchainFramebuffers_.clear();

    for (auto iv : swapchainImageViews_)
        vkDestroyImageView(device_, iv, nullptr);
    swapchainImageViews_.clear();

    if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
}

void VulkanRenderer::recreateSwapchain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window_, &w, &h);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    checkVk(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule),
            "Failed to create shader module");
    return shaderModule;
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties,
                                   VkBuffer& buffer, VkDeviceMemory& memory) const {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    checkVk(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer),
            "Failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    checkVk(vkAllocateMemory(device_, &allocInfo, nullptr, &memory),
            "Failed to allocate buffer memory");

    vkBindBufferMemory(device_, buffer, memory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanRenderer::createImage(uint32_t w, uint32_t h, VkFormat format, VkImageTiling tiling,
                                  VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                                  VkImage& image, VkDeviceMemory& memory) const {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = w;
    imageInfo.extent.height = h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    checkVk(vkCreateImage(device_, &imageInfo, nullptr, &image), "Failed to create image");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    checkVk(vkAllocateMemory(device_, &allocInfo, nullptr, &memory),
            "Failed to allocate image memory");

    vkBindImageMemory(device_, image, memory, 0);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format,
                                             VkImageAspectFlags aspectFlags) const {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    checkVk(vkCreateImageView(device_, &viewInfo, nullptr, &imageView),
            "Failed to create image view");
    return imageView;
}

VkFormat VulkanRenderer::findDepthFormat() const {
    VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }
    throw std::runtime_error("Failed to find supported depth format");
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    checkVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin command buffer");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainExtent_;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{ 0.1f, 0.1f, 0.12f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainExtent_;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             pipelineLayout_, 0, 1, &descriptorSets_[currentFrame_], 0, nullptr);

    // Draw each active entity with its own push-constant model matrix
    for (size_t i = 0; i < entities_.size(); i++) {
        const auto& ent = entities_[i];
        if (!ent.active) continue;

        const MeshData& mesh = meshes_[ent.meshId];

        PushConstantData pc{};
        pc.model = ent.transform;
        vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(PushConstantData), &pc);

        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1,
                         mesh.indexOffset, mesh.vertexOffset, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    checkVk(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer");
}

void VulkanRenderer::setCamera(float eyeX, float eyeY, float eyeZ,
                               float targetX, float targetY, float targetZ,
                               float upX, float upY, float upZ, float fovDegrees) {
    cameraEye_ = glm::vec3(eyeX, eyeY, eyeZ);
    cameraTarget_ = glm::vec3(targetX, targetY, targetZ);
    cameraUp_ = glm::vec3(upX, upY, upZ);
    cameraFov_ = fovDegrees;
}

void VulkanRenderer::getCursorPos(double& x, double& y) const {
    glfwGetCursorPos(window_, &x, &y);
}

void VulkanRenderer::setCursorLocked(bool locked) {
    cursorLocked_ = locked;
    glfwSetInputMode(window_, GLFW_CURSOR,
                     locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool VulkanRenderer::isCursorLocked() const {
    return cursorLocked_;
}

int VulkanRenderer::isMouseButtonPressed(int button) const {
    return glfwGetMouseButton(window_, button) == GLFW_PRESS ? 1 : 0;
}

void VulkanRenderer::getScrollOffset(float& x, float& y) const {
    x = scrollOffsetX_;
    y = scrollOffsetY_;
}

void VulkanRenderer::resetScrollOffset() {
    scrollOffsetX_ = 0.0f;
    scrollOffsetY_ = 0.0f;
}

void VulkanRenderer::updateTime() {
    double now = glfwGetTime();
    if (lastFrameTime_ == 0.0) {
        lastFrameTime_ = now;
        return;
    }
    deltaTime_ = static_cast<float>(now - lastFrameTime_);
    lastFrameTime_ = now;
    totalTime_ += deltaTime_;
}

float VulkanRenderer::getDeltaTime() const { return deltaTime_; }
float VulkanRenderer::getTotalTime() const { return totalTime_; }

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    float aspect = static_cast<float>(swapchainExtent_.width) /
                   static_cast<float>(swapchainExtent_.height);

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(cameraEye_, cameraTarget_, cameraUp_);
    ubo.proj = glm::perspective(glm::radians(cameraFov_), aspect, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1; // Vulkan Y-flip

    memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));

    // Upload light data
    lightData_.cameraPos = glm::vec4(cameraEye_, 1.0f);
    memcpy(lightBuffersMapped_[currentImage], &lightData_, sizeof(lightData_));
}

// ---------------------------------------------------------------------------
// Lighting API
// ---------------------------------------------------------------------------

void VulkanRenderer::setLight(int index, int type,
                              float posX, float posY, float posZ,
                              float dirX, float dirY, float dirZ,
                              float r, float g, float b, float intensity,
                              float radius, float innerCone, float outerCone) {
    if (index < 0 || index >= MAX_LIGHTS) return;

    GpuLight& light = lightData_.lights[index];
    light.position  = glm::vec4(posX, posY, posZ, 0.0f);
    light.direction = glm::vec4(dirX, dirY, dirZ, 0.0f);
    light.color     = glm::vec4(r, g, b, intensity);
    light.innerCone = innerCone;
    light.outerCone = outerCone;
    light.radius    = radius;
    light.type      = type;

    // Recalculate numLights as max active index + 1
    int maxActive = 0;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lightData_.lights[i].color.w > 0.0f || lightData_.lights[i].type >= 0) {
            // Check if this slot has been explicitly set (intensity > 0)
            if (lightData_.lights[i].color.w > 0.0f)
                maxActive = i + 1;
        }
    }
    lightData_.numLights = maxActive;
}

void VulkanRenderer::clearLight(int index) {
    if (index < 0 || index >= MAX_LIGHTS) return;

    lightData_.lights[index] = GpuLight{};

    // Recalculate numLights
    int maxActive = 0;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lightData_.lights[i].color.w > 0.0f)
            maxActive = i + 1;
    }
    lightData_.numLights = maxActive;
}

void VulkanRenderer::setAmbientIntensity(float intensity) {
    lightData_.ambientIntensity = intensity;
}

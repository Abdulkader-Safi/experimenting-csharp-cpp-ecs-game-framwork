#include "renderer.h"
#include <iostream>

static VulkanRenderer g_renderer;

extern "C" {

bool renderer_init(int width, int height, const char* title) {
    try {
        return g_renderer.init(width, height, title);
    } catch (const std::exception& e) {
        std::cerr << "renderer_init error: " << e.what() << std::endl;
        return false;
    }
}

void renderer_cleanup() {
    try {
        g_renderer.cleanup();
    } catch (const std::exception& e) {
        std::cerr << "renderer_cleanup error: " << e.what() << std::endl;
    }
}

bool renderer_load_model(const char* path) {
    try {
        return g_renderer.loadModel(path);
    } catch (const std::exception& e) {
        std::cerr << "renderer_load_model error: " << e.what() << std::endl;
        return false;
    }
}

bool renderer_should_close() {
    return g_renderer.shouldClose();
}

void renderer_poll_events() {
    g_renderer.pollEvents();
}

int renderer_is_key_pressed(int glfw_key) {
    return g_renderer.isKeyPressed(glfw_key);
}

void renderer_set_rotation(float rx, float ry, float rz) {
    g_renderer.setRotation(rx, ry, rz);
}

void renderer_render_frame() {
    try {
        g_renderer.renderFrame();
    } catch (const std::exception& e) {
        std::cerr << "renderer_render_frame error: " << e.what() << std::endl;
    }
}

// --- Multi-entity API ---

int renderer_load_mesh(const char* path) {
    try {
        return g_renderer.loadMesh(path);
    } catch (const std::exception& e) {
        std::cerr << "renderer_load_mesh error: " << e.what() << std::endl;
        return -1;
    }
}

int renderer_create_entity(int mesh_id) {
    try {
        return g_renderer.createEntity(mesh_id);
    } catch (const std::exception& e) {
        std::cerr << "renderer_create_entity error: " << e.what() << std::endl;
        return -1;
    }
}

void renderer_set_entity_transform(int entity_id, const float* mat4x4) {
    try {
        g_renderer.setEntityTransform(entity_id, mat4x4);
    } catch (const std::exception& e) {
        std::cerr << "renderer_set_entity_transform error: " << e.what() << std::endl;
    }
}

void renderer_remove_entity(int entity_id) {
    try {
        g_renderer.removeEntity(entity_id);
    } catch (const std::exception& e) {
        std::cerr << "renderer_remove_entity error: " << e.what() << std::endl;
    }
}

} // extern "C"

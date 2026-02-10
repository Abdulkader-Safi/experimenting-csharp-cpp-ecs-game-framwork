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

void renderer_set_camera(float eyeX, float eyeY, float eyeZ,
                         float targetX, float targetY, float targetZ,
                         float upX, float upY, float upZ, float fovDegrees) {
    g_renderer.setCamera(eyeX, eyeY, eyeZ, targetX, targetY, targetZ,
                         upX, upY, upZ, fovDegrees);
}

// --- Cursor API ---

void renderer_get_cursor_pos(double* x, double* y) {
    g_renderer.getCursorPos(*x, *y);
}

void renderer_set_cursor_locked(int locked) {
    g_renderer.setCursorLocked(locked != 0);
}

int renderer_is_cursor_locked() {
    return g_renderer.isCursorLocked() ? 1 : 0;
}

// --- Mouse API ---

int renderer_is_mouse_button_pressed(int button) {
    return g_renderer.isMouseButtonPressed(button);
}

void renderer_get_scroll_offset(float* x, float* y) {
    g_renderer.getScrollOffset(*x, *y);
}

void renderer_reset_scroll_offset() {
    g_renderer.resetScrollOffset();
}

// --- Time API ---

void renderer_update_time() {
    g_renderer.updateTime();
}

float renderer_get_delta_time() {
    return g_renderer.getDeltaTime();
}

float renderer_get_total_time() {
    return g_renderer.getTotalTime();
}

// --- Lighting API ---

void renderer_set_light(int index, int type,
                        float posX, float posY, float posZ,
                        float dirX, float dirY, float dirZ,
                        float r, float g, float b, float intensity,
                        float radius, float innerCone, float outerCone) {
    g_renderer.setLight(index, type, posX, posY, posZ, dirX, dirY, dirZ,
                        r, g, b, intensity, radius, innerCone, outerCone);
}

void renderer_clear_light(int index) {
    g_renderer.clearLight(index);
}

void renderer_set_ambient(float intensity) {
    g_renderer.setAmbientIntensity(intensity);
}

} // extern "C"

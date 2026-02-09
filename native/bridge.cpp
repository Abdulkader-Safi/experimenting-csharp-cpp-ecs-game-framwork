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

} // extern "C"

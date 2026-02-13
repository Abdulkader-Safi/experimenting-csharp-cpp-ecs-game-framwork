#include "renderer.h"
#include <iostream>

static VulkanRenderer g_renderer;

// Bridge guard: wraps a renderer call in try/catch, logs errors, returns
// fallback on failure. Eliminates repetitive try/catch boilerplate across all
// bridge functions that can throw.
#define BRIDGE_GUARD(fallback, call)                                           \
  try {                                                                        \
    return call;                                                               \
  } catch (const std::exception &e) {                                          \
    std::cerr << __func__ << " error: " << e.what() << std::endl;              \
    return fallback;                                                           \
  }

#define BRIDGE_GUARD_VOID(call)                                                \
  try {                                                                        \
    call;                                                                      \
  } catch (const std::exception &e) {                                          \
    std::cerr << __func__ << " error: " << e.what() << std::endl;              \
  }

extern "C" {

// --- Core lifecycle ---

bool renderer_init(int width, int height, const char *title) {
  BRIDGE_GUARD(false, g_renderer.init(width, height, title))
}

void renderer_cleanup() { BRIDGE_GUARD_VOID(g_renderer.cleanup()) }

bool renderer_load_model(const char *path) {
  BRIDGE_GUARD(false, g_renderer.loadModel(path))
}

bool renderer_should_close() { return g_renderer.shouldClose(); }

void renderer_poll_events() { g_renderer.pollEvents(); }

int renderer_is_key_pressed(int glfw_key) {
  return g_renderer.isKeyPressed(glfw_key);
}

void renderer_set_rotation(float rx, float ry, float rz) {
  g_renderer.setRotation(rx, ry, rz);
}

void renderer_render_frame() { BRIDGE_GUARD_VOID(g_renderer.renderFrame()) }

// --- Multi-entity API ---

int renderer_load_mesh(const char *path) {
  BRIDGE_GUARD(-1, g_renderer.loadMesh(path))
}

int renderer_create_entity(int mesh_id) {
  BRIDGE_GUARD(-1, g_renderer.createEntity(mesh_id))
}

void renderer_set_entity_transform(int entity_id, const float *mat4x4) {
  BRIDGE_GUARD_VOID(g_renderer.setEntityTransform(entity_id, mat4x4))
}

void renderer_remove_entity(int entity_id) {
  BRIDGE_GUARD_VOID(g_renderer.removeEntity(entity_id))
}

void renderer_set_camera(float eyeX, float eyeY, float eyeZ, float targetX,
                         float targetY, float targetZ, float upX, float upY,
                         float upZ, float fovDegrees) {
  g_renderer.setCamera(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY,
                       upZ, fovDegrees);
}

// --- Cursor API ---

void renderer_get_cursor_pos(double *x, double *y) {
  g_renderer.getCursorPos(*x, *y);
}

void renderer_set_cursor_locked(int locked) {
  g_renderer.setCursorLocked(locked != 0);
}

int renderer_is_cursor_locked() { return g_renderer.isCursorLocked() ? 1 : 0; }

// --- Mouse API ---

int renderer_is_mouse_button_pressed(int button) {
  return g_renderer.isMouseButtonPressed(button);
}

void renderer_get_scroll_offset(float *x, float *y) {
  g_renderer.getScrollOffset(*x, *y);
}

void renderer_reset_scroll_offset() { g_renderer.resetScrollOffset(); }

// --- Time API ---

void renderer_update_time() { g_renderer.updateTime(); }

float renderer_get_delta_time() { return g_renderer.getDeltaTime(); }

float renderer_get_total_time() { return g_renderer.getTotalTime(); }

// --- Procedural Primitives API ---

int renderer_create_box_mesh(float w, float h, float l, float r, float g,
                             float b) {
  BRIDGE_GUARD(-1, g_renderer.createBoxMesh(w, h, l, r, g, b))
}

int renderer_create_sphere_mesh(float radius, int segments, int rings, float r,
                                float g, float b) {
  BRIDGE_GUARD(-1,
               g_renderer.createSphereMesh(radius, segments, rings, r, g, b))
}

int renderer_create_plane_mesh(float w, float h, float r, float g, float b) {
  BRIDGE_GUARD(-1, g_renderer.createPlaneMesh(w, h, r, g, b))
}

int renderer_create_cylinder_mesh(float radius, float height, int segments,
                                  float r, float g, float b) {
  BRIDGE_GUARD(-1,
               g_renderer.createCylinderMesh(radius, height, segments, r, g, b))
}

int renderer_create_capsule_mesh(float radius, float height, int segments,
                                 int rings, float r, float g, float b) {
  BRIDGE_GUARD(-1, g_renderer.createCapsuleMesh(radius, height, segments, rings,
                                                r, g, b))
}

// --- Lighting API ---

void renderer_set_light(int index, int type, float posX, float posY, float posZ,
                        float dirX, float dirY, float dirZ, float r, float g,
                        float b, float intensity, float radius, float innerCone,
                        float outerCone) {
  g_renderer.setLight(index, type, posX, posY, posZ, dirX, dirY, dirZ, r, g, b,
                      intensity, radius, innerCone, outerCone);
}

void renderer_clear_light(int index) { g_renderer.clearLight(index); }

void renderer_set_ambient(float intensity) {
  g_renderer.setAmbientIntensity(intensity);
}

// --- Debug Overlay API ---

void renderer_set_debug_overlay(int enabled) {
  g_renderer.setDebugOverlay(enabled != 0);
}

int renderer_get_entity_count() { return g_renderer.getActiveEntityCount(); }

// --- Debug Wireframe Entity API ---

int renderer_create_debug_entity(int mesh_id) {
  BRIDGE_GUARD(-1, g_renderer.createDebugEntity(mesh_id))
}

void renderer_set_debug_entity_transform(int entity_id, const float *mat4x4) {
  BRIDGE_GUARD_VOID(g_renderer.setDebugEntityTransform(entity_id, mat4x4))
}

void renderer_remove_debug_entity(int entity_id) {
  BRIDGE_GUARD_VOID(g_renderer.removeDebugEntity(entity_id))
}

void renderer_clear_debug_entities() {
  BRIDGE_GUARD_VOID(g_renderer.clearDebugEntities())
}

} // extern "C"

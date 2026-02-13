using System;
using System.Runtime.InteropServices;

namespace ECS
{
    public static class NativeBridge
    {
        const string LIB = "renderer";

        // GLFW key codes
        public const int GLFW_KEY_RIGHT = 262;
        public const int GLFW_KEY_LEFT = 263;
        public const int GLFW_KEY_DOWN = 264;
        public const int GLFW_KEY_UP = 265;
        public const int GLFW_KEY_W = 87;
        public const int GLFW_KEY_A = 65;
        public const int GLFW_KEY_S = 83;
        public const int GLFW_KEY_D = 68;
        public const int GLFW_KEY_Q = 81;
        public const int GLFW_KEY_E = 69;
        public const int GLFW_KEY_R = 82;
        public const int GLFW_KEY_F = 70;
        public const int GLFW_KEY_ESCAPE = 256;
        public const int GLFW_KEY_TAB = 258;
        public const int GLFW_KEY_0 = 48;
        public const int GLFW_KEY_1 = 49;

        // GLFW mouse button codes
        public const int GLFW_MOUSE_BUTTON_LEFT = 0;
        public const int GLFW_MOUSE_BUTTON_RIGHT = 1;
        public const int GLFW_MOUSE_BUTTON_MIDDLE = 2;

        // Legacy API
        [DllImport(LIB)] public static extern bool renderer_init(int width, int height, string title);
        [DllImport(LIB)] public static extern void renderer_cleanup();
        [DllImport(LIB)] public static extern bool renderer_load_model(string path);
        [DllImport(LIB)] public static extern bool renderer_should_close();
        [DllImport(LIB)] public static extern void renderer_poll_events();
        [DllImport(LIB)] public static extern int renderer_is_key_pressed(int glfw_key);
        [DllImport(LIB)] public static extern void renderer_set_rotation(float rx, float ry, float rz);
        [DllImport(LIB)] public static extern void renderer_render_frame();

        // Multi-entity API
        [DllImport(LIB)] public static extern int renderer_load_mesh(string path);
        [DllImport(LIB)] public static extern int renderer_create_entity(int mesh_id);
        [DllImport(LIB)] public static extern void renderer_set_entity_transform(int entity_id, float[] mat4x4);
        [DllImport(LIB)] public static extern void renderer_remove_entity(int entity_id);

        // Camera API
        [DllImport(LIB)]
        public static extern void renderer_set_camera(
            float eyeX, float eyeY, float eyeZ,
            float targetX, float targetY, float targetZ,
            float upX, float upY, float upZ, float fovDegrees);

        // Cursor API
        [DllImport(LIB)] public static extern void renderer_get_cursor_pos(out double x, out double y);
        [DllImport(LIB)] public static extern void renderer_set_cursor_locked(int locked);
        [DllImport(LIB)] public static extern int renderer_is_cursor_locked();

        // Mouse API
        [DllImport(LIB)] public static extern int renderer_is_mouse_button_pressed(int button);
        [DllImport(LIB)] public static extern void renderer_get_scroll_offset(out float x, out float y);
        [DllImport(LIB)] public static extern void renderer_reset_scroll_offset();

        // Time API
        [DllImport(LIB)] public static extern void renderer_update_time();
        [DllImport(LIB)] public static extern float renderer_get_delta_time();
        [DllImport(LIB)] public static extern float renderer_get_total_time();

        // Procedural Primitives API
        [DllImport(LIB)] public static extern int renderer_create_box_mesh(float w, float h, float l, float r, float g, float b);
        [DllImport(LIB)] public static extern int renderer_create_sphere_mesh(float radius, int segments, int rings, float r, float g, float b);
        [DllImport(LIB)] public static extern int renderer_create_plane_mesh(float w, float h, float r, float g, float b);
        [DllImport(LIB)] public static extern int renderer_create_cylinder_mesh(float radius, float height, int segments, float r, float g, float b);
        [DllImport(LIB)] public static extern int renderer_create_capsule_mesh(float radius, float height, int segments, int rings, float r, float g, float b);

        // Debug Overlay API
        [DllImport(LIB)] public static extern void renderer_set_debug_overlay(int enabled);
        [DllImport(LIB)] public static extern int renderer_get_entity_count();

        // Lighting API
        [DllImport(LIB)]
        public static extern void renderer_set_light(
            int index, int type,
            float posX, float posY, float posZ,
            float dirX, float dirY, float dirZ,
            float r, float g, float b, float intensity,
            float radius, float innerCone, float outerCone);
        [DllImport(LIB)] public static extern void renderer_clear_light(int index);
        [DllImport(LIB)] public static extern void renderer_set_ambient(float intensity);

        // Convenience wrappers
        public static bool IsKeyPressed(int key)
        {
            return renderer_is_key_pressed(key) != 0;
        }

        public static int LoadMesh(string path)
        {
            return renderer_load_mesh(path);
        }

        public static int CreateEntity(int meshId)
        {
            return renderer_create_entity(meshId);
        }

        public static void SetEntityTransform(int entityId, float[] matrix)
        {
            renderer_set_entity_transform(entityId, matrix);
        }

        public static void RemoveEntity(int entityId)
        {
            renderer_remove_entity(entityId);
        }

        public static void SetCamera(float eyeX, float eyeY, float eyeZ,
                                      float targetX, float targetY, float targetZ,
                                      float upX, float upY, float upZ, float fovDegrees)
        {
            renderer_set_camera(eyeX, eyeY, eyeZ, targetX, targetY, targetZ,
                                upX, upY, upZ, fovDegrees);
        }

        public static void GetCursorPos(out double x, out double y)
        {
            renderer_get_cursor_pos(out x, out y);
        }

        public static void SetCursorLocked(bool locked)
        {
            renderer_set_cursor_locked(locked ? 1 : 0);
        }

        public static bool IsCursorLocked()
        {
            return renderer_is_cursor_locked() != 0;
        }

        public static void SetLight(int index, int type,
                                     float posX, float posY, float posZ,
                                     float dirX, float dirY, float dirZ,
                                     float r, float g, float b, float intensity,
                                     float radius, float innerCone, float outerCone)
        {
            renderer_set_light(index, type, posX, posY, posZ, dirX, dirY, dirZ,
                               r, g, b, intensity, radius, innerCone, outerCone);
        }

        public static void ClearLight(int index)
        {
            renderer_clear_light(index);
        }

        public static void SetAmbient(float intensity)
        {
            renderer_set_ambient(intensity);
        }

        // Procedural Primitives — full parameter versions
        public static int CreateBoxMesh(float w, float h, float l, float r, float g, float b)
        {
            return renderer_create_box_mesh(w, h, l, r, g, b);
        }

        public static int CreateSphereMesh(float radius, int segments, int rings, float r, float g, float b)
        {
            return renderer_create_sphere_mesh(radius, segments, rings, r, g, b);
        }

        public static int CreatePlaneMesh(float w, float h, float r, float g, float b)
        {
            return renderer_create_plane_mesh(w, h, r, g, b);
        }

        public static int CreateCylinderMesh(float radius, float height, int segments, float r, float g, float b)
        {
            return renderer_create_cylinder_mesh(radius, height, segments, r, g, b);
        }

        public static int CreateCapsuleMesh(float radius, float height, int segments, int rings, float r, float g, float b)
        {
            return renderer_create_capsule_mesh(radius, height, segments, rings, r, g, b);
        }

        // Procedural Primitives — convenience overloads (default color=grey 0.7, segments=32, rings=16)
        public static int CreateBoxMesh(float w, float h, float l)
        {
            return renderer_create_box_mesh(w, h, l, 0.7f, 0.7f, 0.7f);
        }

        public static int CreateSphereMesh(float radius)
        {
            return renderer_create_sphere_mesh(radius, 32, 16, 0.7f, 0.7f, 0.7f);
        }

        public static int CreatePlaneMesh(float w, float h)
        {
            return renderer_create_plane_mesh(w, h, 0.7f, 0.7f, 0.7f);
        }

        public static int CreateCylinderMesh(float radius, float height)
        {
            return renderer_create_cylinder_mesh(radius, height, 32, 0.7f, 0.7f, 0.7f);
        }

        public static int CreateCapsuleMesh(float radius, float height)
        {
            return renderer_create_capsule_mesh(radius, height, 32, 16, 0.7f, 0.7f, 0.7f);
        }

        public static void SetDebugOverlay(bool enabled)
        {
            renderer_set_debug_overlay(enabled ? 1 : 0);
        }

        public static int GetEntityCount()
        {
            return renderer_get_entity_count();
        }

        public static bool IsMouseButtonPressed(int button)
        {
            return renderer_is_mouse_button_pressed(button) != 0;
        }

        public static void GetScrollOffset(out float x, out float y)
        {
            renderer_get_scroll_offset(out x, out y);
        }

        public static void ResetScrollOffset()
        {
            renderer_reset_scroll_offset();
        }
    }
}

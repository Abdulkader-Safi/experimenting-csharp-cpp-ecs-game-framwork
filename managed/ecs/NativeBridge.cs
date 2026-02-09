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
    }
}

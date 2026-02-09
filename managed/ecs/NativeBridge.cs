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
    }
}

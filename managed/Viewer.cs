using System;
using System.Runtime.InteropServices;

class Viewer
{
    const string LIB = "renderer";

    // GLFW key codes
    const int GLFW_KEY_RIGHT = 262;
    const int GLFW_KEY_LEFT = 263;
    const int GLFW_KEY_DOWN = 264;
    const int GLFW_KEY_UP = 265;
    const int GLFW_KEY_W = 87;
    const int GLFW_KEY_A = 65;
    const int GLFW_KEY_S = 83;
    const int GLFW_KEY_D = 68;


    [DllImport(LIB)] static extern bool renderer_init(int width, int height, string title);
    [DllImport(LIB)] static extern void renderer_cleanup();
    [DllImport(LIB)] static extern bool renderer_load_model(string path);
    [DllImport(LIB)] static extern bool renderer_should_close();
    [DllImport(LIB)] static extern void renderer_poll_events();
    [DllImport(LIB)] static extern int renderer_is_key_pressed(int glfw_key);
    [DllImport(LIB)] static extern void renderer_set_rotation(float rx, float ry, float rz);
    [DllImport(LIB)] static extern void renderer_render_frame();

    // Set model path here
    static string ModelPath = "models/AnimationLibrary_Godot_Standard.glb";

    static void Main(string[] args)
    {
        string modelPath = args.Length > 0 ? args[0] : ModelPath;

        if (!renderer_init(800, 600, "Vulkan glTF Viewer"))
        {
            Console.WriteLine("Failed to initialize renderer");
            return;
        }

        if (!renderer_load_model(modelPath))
        {
            Console.WriteLine("Failed to load model: " + modelPath);
            renderer_cleanup();
            return;
        }

        float rotX = 0.0f;
        float rotY = 0.0f;

        while (!renderer_should_close())
        {
            renderer_poll_events();

            if (renderer_is_key_pressed(GLFW_KEY_LEFT) != 0 || renderer_is_key_pressed(GLFW_KEY_A) != 0) rotY -= 1.5f;
            if (renderer_is_key_pressed(GLFW_KEY_RIGHT) != 0 || renderer_is_key_pressed(GLFW_KEY_D) != 0) rotY += 1.5f;
            if (renderer_is_key_pressed(GLFW_KEY_UP) != 0 || renderer_is_key_pressed(GLFW_KEY_W) != 0) rotX -= 1.5f;
            if (renderer_is_key_pressed(GLFW_KEY_DOWN) != 0 || renderer_is_key_pressed(GLFW_KEY_S) != 0) rotX += 1.5f;

            renderer_set_rotation(rotX, rotY, 0.0f);
            renderer_render_frame();
        }

        renderer_cleanup();
    }
}

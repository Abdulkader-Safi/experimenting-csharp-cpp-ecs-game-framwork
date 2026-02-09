using System;
using ECS;

class Viewer
{
    static readonly string ModelPath = "models/AnimationLibrary_Godot_Standard.glb";

    static void Main(string[] args)
    {
        string modelPath = args.Length > 0 ? args[0] : ModelPath;

        if (!NativeBridge.renderer_init(800, 600, "Vulkan glTF Viewer"))
        {
            Console.WriteLine("Failed to initialize renderer");
            return;
        }

        // Create ECS world
        var world = new World();

        // Load mesh
        int meshId = NativeBridge.LoadMesh(modelPath);
        if (meshId < 0)
        {
            Console.WriteLine("Failed to load model: " + modelPath);
            NativeBridge.renderer_cleanup();
            return;
        }

        // Spawn movable entity (controlled by WASD)
        int player = world.Spawn();
        world.AddComponent(player, new Transform());
        var playerMesh = new MeshComponent
        {
            MeshId = meshId,
            RendererEntityId = NativeBridge.CreateEntity(meshId)
        };
        world.AddComponent(player, playerMesh);
        world.AddComponent(player, new Movable
        {
            Speed = 1.5f
        });

        // Spawn a second static entity offset to the right
        int prop = world.Spawn();
        world.AddComponent(prop, new Transform
        {
            X = 2.5f
        });
        var propMesh = new MeshComponent
        {
            MeshId = meshId,
            RendererEntityId = NativeBridge.CreateEntity(meshId)
        };
        world.AddComponent(prop, propMesh);

        // Register systems
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.RenderSyncSystem);

        // Main loop
        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_poll_events();
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

        NativeBridge.renderer_cleanup();
    }
}

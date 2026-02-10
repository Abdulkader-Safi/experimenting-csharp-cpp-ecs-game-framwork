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
            Speed = 90f
        });
        world.AddComponent(player, new Camera());

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

        // Spawn directional sun light
        int sun = world.Spawn();
        world.AddComponent(sun, new Transform());
        world.AddComponent(sun, new Light
        {
            Type = Light.Directional,
            DirX = 1f,
            DirY = 1f,
            DirZ = 1f,
            Intensity = 1.0f
        });

        NativeBridge.SetAmbient(0.15f);

        // Register systems
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.LightSyncSystem);
        world.AddSystem(Systems.RenderSyncSystem);

        // Main loop
        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_poll_events();
            world.UpdateTime();
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

        NativeBridge.renderer_cleanup();
    }
}

using System;
using ECS;

public static class Game
{
    static readonly string ModelPath = "models/AnimationLibrary_Godot_Standard.glb";

    public static void Setup(World world)
    {
        // Initialize physics (idempotent)
        PhysicsWorld.Instance.Init();

        // Load mesh
        int meshId = NativeBridge.LoadMesh(ModelPath);
        if (meshId < 0)
        {
            Console.WriteLine("Failed to load model: " + ModelPath);
            return;
        }

        // Spawn movable entity (controlled by WASD)
        int player = world.Spawn();
        world.AddComponent(player, new Rigidbody { Friction = 0.3f, Restitution = 0.7f, GravityFactor = 0.5f });
        world.AddComponent(player, new Collider { ShapeType = Collider.Capsule, CapsuleRadius = 0.2f, DebugColor = Color.Blue });
        world.AddComponent(player, new Transform());

        // Second collider as child entity (box wireframe, follows player)
        int playerBox = world.Spawn();
        world.AddComponent(playerBox, new Transform());
        world.AddComponent(playerBox, new Hierarchy { Parent = player });
        world.AddComponent(playerBox, new Collider { ShapeType = Collider.Box, DebugColor = new Color("#ff6600") });
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

        // --- Procedural primitives showcase ---

        // Ground plane
        int groundMesh = NativeBridge.CreatePlaneMesh(20f, 20f, 0.3f, 0.3f, 0.3f);
        int ground = world.Spawn();
        world.AddComponent(ground, new Transform { Y = -1f });
        world.AddComponent(ground, new MeshComponent
        {
            MeshId = groundMesh,
            RendererEntityId = NativeBridge.CreateEntity(groundMesh)
        });

        // Red box
        int boxMesh = NativeBridge.CreateBoxMesh(1f, 1f, 1f, 0.8f, 0.2f, 0.2f);
        int box = world.Spawn();
        world.AddComponent(box, new Transform { X = -4f, Y = -0.5f, Z = 0f });
        world.AddComponent(box, new MeshComponent
        {
            MeshId = boxMesh,
            RendererEntityId = NativeBridge.CreateEntity(boxMesh)
        });

        // Green sphere
        int sphereMesh = NativeBridge.CreateSphereMesh(0.5f, 32, 16, 0.2f, 0.8f, 0.2f);
        int sphere = world.Spawn();
        world.AddComponent(sphere, new Transform { X = -6f, Y = -0.5f, Z = 0f });
        world.AddComponent(sphere, new MeshComponent
        {
            MeshId = sphereMesh,
            RendererEntityId = NativeBridge.CreateEntity(sphereMesh)
        });

        // Blue cylinder
        int cylMesh = NativeBridge.CreateCylinderMesh(0.4f, 1f, 32, 0.2f, 0.2f, 0.8f);
        int cyl = world.Spawn();
        world.AddComponent(cyl, new Transform { X = -8f, Y = -0.5f, Z = 0f });
        world.AddComponent(cyl, new MeshComponent
        {
            MeshId = cylMesh,
            RendererEntityId = NativeBridge.CreateEntity(cylMesh)
        });

        // Yellow capsule
        int capMesh = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f, 32, 16, 0.9f, 0.8f, 0.2f);
        int cap = world.Spawn();
        world.AddComponent(cap, new Transform { X = -10f, Y = -0.5f, Z = 0f });
        world.AddComponent(cap, new MeshComponent
        {
            MeshId = capMesh,
            RendererEntityId = NativeBridge.CreateEntity(capMesh)
        });

        // --- Physics demo entities ---

        // Physics ground plane (static)
        int physGround = world.Spawn();
        world.AddComponent(physGround, new Transform { Y = -1f });
        world.AddComponent(physGround, new Rigidbody { MotionType = JPH_MotionType.Static, Friction = 0.8f });
        world.AddComponent(physGround, new Collider { ShapeType = Collider.Plane, PlaneHalfExtent = 100f });

        PhysicsWorld.Instance.OptimizeBroadPhase();

        // Register systems (order matters - RenderSync always last)
        world.AddSystem(Systems.InputMovementSystem);
        world.AddSystem(Systems.TimerSystem);
        world.AddSystem(Systems.PhysicsSystem);
        world.AddSystem(Systems.FreeCameraSystem);
        world.AddSystem(Systems.CameraFollowSystem);
        world.AddSystem(Systems.LightSyncSystem);
        world.AddSystem(Systems.HierarchyTransformSystem);
        world.AddSystem(Systems.DebugOverlaySystem);
        world.AddSystem(Systems.DebugColliderRenderSystem);
        world.AddSystem(Systems.RenderSyncSystem);
    }
}

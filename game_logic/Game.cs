using System;
using ECS;

public static class Game
{
    static readonly string ModelPath = "models/AnimationLibrary_Godot_Standard.glb";

    public static void Setup(World world)
    {
        PhysicsWorld.Instance.Init();

        int meshId = NativeBridge.LoadMesh(ModelPath);
        if (meshId < 0)
        {
            Console.WriteLine("Failed to load model: " + ModelPath);
            return;
        }

        // Spawn movable entity (controlled by WASD)
        int player = world.SpawnMeshEntity(meshId, new Transform());
        world.AddComponent(player, new Rigidbody { Friction = 0.3f, Restitution = 0.7f, GravityFactor = 0.5f });
        world.AddComponent(player, new Collider { ShapeType = Collider.Capsule, CapsuleRadius = 0.2f, DebugColor = Color.Blue });
        world.AddComponent(player, new Movable { Speed = 90f });
        world.AddComponent(player, new Camera());

        // Second collider as child entity (box wireframe, follows player)
        int playerBox = world.Spawn();
        world.AddComponent(playerBox, new Transform());
        world.AddComponent(playerBox, new Hierarchy { Parent = player });
        world.AddComponent(playerBox, new Collider { ShapeType = Collider.Box, DebugColor = new Color("#ff6600") });

        // Static prop entity offset to the right
        world.SpawnMeshEntity(meshId, new Transform { X = 2.5f });

        // Directional sun light
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
        int groundMesh = NativeBridge.CreatePlaneMesh(20f, 20f, 0.3f, 0.3f, 0.3f);
        world.SpawnMeshEntity(groundMesh, new Transform { Y = -1f });

        int boxMesh = NativeBridge.CreateBoxMesh(1f, 1f, 1f, 0.8f, 0.2f, 0.2f);
        world.SpawnMeshEntity(boxMesh, new Transform { X = -4f, Y = -0.5f });

        int sphereMesh = NativeBridge.CreateSphereMesh(0.5f, 32, 16, 0.2f, 0.8f, 0.2f);
        world.SpawnMeshEntity(sphereMesh, new Transform { X = -6f, Y = -0.5f });

        int cylMesh = NativeBridge.CreateCylinderMesh(0.4f, 1f, 32, 0.2f, 0.2f, 0.8f);
        world.SpawnMeshEntity(cylMesh, new Transform { X = -8f, Y = -0.5f });

        int capMesh = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f, 32, 16, 0.9f, 0.8f, 0.2f);
        world.SpawnMeshEntity(capMesh, new Transform { X = -10f, Y = -0.5f });

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

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
        world.AddComponent(player, new Collider { Shape = ShapeType.Capsule, CapsuleRadius = 0.2f, DebugColor = Color.Blue });
        world.AddComponent(player, new Movable { Speed = 90f });
        world.AddComponent(player, new Camera());

        // Second collider as child entity (box wireframe, follows player)
        int playerBox = world.Spawn(
            new Transform(),
            new Hierarchy { Parent = player },
            new Collider { Shape = ShapeType.Box, DebugColor = new Color("#ff6600") }
        );

        // Static prop entity offset to the right
        world.SpawnMeshEntity(meshId, new Transform { Position = new Vec3(2.5f, 0f, 0f) });

        // Directional sun light
        int sun = world.Spawn(
            new Transform(),
            new Light
            {
                Type = LightType.Directional,
                Direction = new Vec3(1f, 1f, 1f),
                Intensity = 1.0f
            }
        );

        NativeBridge.SetAmbient(0.15f);

        // --- Procedural primitives showcase ---
        int groundMesh = NativeBridge.CreatePlaneMesh(20f, 20f, new Color(0.3f, 0.3f, 0.3f));
        world.SpawnMeshEntity(groundMesh, new Transform { Position = new Vec3(0f, -1f, 0f) });

        int boxMesh = NativeBridge.CreateBoxMesh(1f, 1f, 1f, new Color(0.8f, 0.2f, 0.2f));
        world.SpawnMeshEntity(boxMesh, new Transform { Position = new Vec3(-4f, -0.5f, 0f) });

        int sphereMesh = NativeBridge.CreateSphereMesh(0.5f, 32, 16, new Color(0.2f, 0.8f, 0.2f));
        world.SpawnMeshEntity(sphereMesh, new Transform { Position = new Vec3(-6f, -0.5f, 0f) });

        int cylMesh = NativeBridge.CreateCylinderMesh(0.4f, 1f, 32, new Color(0.2f, 0.2f, 0.8f));
        world.SpawnMeshEntity(cylMesh, new Transform { Position = new Vec3(-8f, -0.5f, 0f) });

        int capMesh = NativeBridge.CreateCapsuleMesh(0.3f, 0.6f, 32, 16, new Color(0.9f, 0.8f, 0.2f));
        world.SpawnMeshEntity(capMesh, new Transform { Position = new Vec3(-10f, -0.5f, 0f) });

        // --- Physics demo entities ---

        // Physics ground plane (static)
        int physGround = world.Spawn(
            new Transform { Position = new Vec3(0f, -1f, 0f) },
            new Rigidbody { MotionType = JPH_MotionType.Static, Friction = 0.8f },
            new Collider { Shape = ShapeType.Plane, PlaneHalfExtent = 100f }
        );

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

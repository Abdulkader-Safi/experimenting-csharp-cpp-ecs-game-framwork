using System;
using System.Runtime.InteropServices;

namespace ECS
{
    [StructLayout(LayoutKind.Sequential)]
    public struct JPH_Vec3
    {
        public float x, y, z;

        public JPH_Vec3(float x, float y, float z)
        {
            this.x = x; this.y = y; this.z = z;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JPH_RVec3
    {
        public double x, y, z;

        public JPH_RVec3(double x, double y, double z)
        {
            this.x = x; this.y = y; this.z = z;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JPH_Quat
    {
        public float x, y, z, w;

        public JPH_Quat(float x, float y, float z, float w)
        {
            this.x = x; this.y = y; this.z = z; this.w = w;
        }

        public static JPH_Quat Identity { get { return new JPH_Quat(0f, 0f, 0f, 1f); } }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JPH_Plane
    {
        public JPH_Vec3 normal;
        public float distance;

        public JPH_Plane(JPH_Vec3 normal, float distance)
        {
            this.normal = normal;
            this.distance = distance;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JobSystemThreadPoolConfig
    {
        public uint maxJobs;
        public uint maxBarriers;
        public int numThreads;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct JPH_PhysicsSystemSettings
    {
        public uint maxBodies;
        public uint numBodyMutexes;
        public uint maxBodyPairs;
        public uint maxContactConstraints;
        public uint _padding;
        public IntPtr broadPhaseLayerInterface;
        public IntPtr objectLayerPairFilter;
        public IntPtr objectVsBroadPhaseLayerFilter;
    }

    public enum JPH_MotionType
    {
        Static = 0,
        Kinematic = 1,
        Dynamic = 2
    }

    public enum JPH_Activation
    {
        Activate = 0,
        DontActivate = 1
    }

    public static class PhysicsBridge
    {
        const string LIB = "joltc";

        // Init / Shutdown
        [DllImport(LIB)] public static extern bool JPH_Init();
        [DllImport(LIB)] public static extern void JPH_Shutdown();

        // Job system
        [DllImport(LIB)] public static extern IntPtr JPH_JobSystemThreadPool_Create(ref JobSystemThreadPoolConfig config);
        [DllImport(LIB)] public static extern void JPH_JobSystem_Destroy(IntPtr jobSystem);

        // Object layer pair filter
        [DllImport(LIB)] public static extern IntPtr JPH_ObjectLayerPairFilterTable_Create(uint numObjectLayers);
        [DllImport(LIB)] public static extern void JPH_ObjectLayerPairFilterTable_EnableCollision(IntPtr filter, uint layer1, uint layer2);

        // Broad phase layer interface
        [DllImport(LIB)] public static extern IntPtr JPH_BroadPhaseLayerInterfaceTable_Create(uint numObjectLayers, uint numBroadPhaseLayers);
        [DllImport(LIB)] public static extern void JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(IntPtr bpInterface, uint objectLayer, byte broadPhaseLayer);

        // Object vs broad phase layer filter
        [DllImport(LIB)] public static extern IntPtr JPH_ObjectVsBroadPhaseLayerFilterTable_Create(IntPtr broadPhaseLayerInterface, uint numBroadPhaseLayers, IntPtr objectLayerPairFilter, uint numObjectLayers);

        // Physics system
        [DllImport(LIB)] public static extern IntPtr JPH_PhysicsSystem_Create(ref JPH_PhysicsSystemSettings settings);
        [DllImport(LIB)] public static extern void JPH_PhysicsSystem_Destroy(IntPtr system);
        [DllImport(LIB)] public static extern int JPH_PhysicsSystem_Update(IntPtr system, float deltaTime, int collisionSteps, IntPtr jobSystem);
        [DllImport(LIB)] public static extern void JPH_PhysicsSystem_OptimizeBroadPhase(IntPtr system);
        [DllImport(LIB)] public static extern IntPtr JPH_PhysicsSystem_GetBodyInterface(IntPtr system);
        [DllImport(LIB)] public static extern void JPH_PhysicsSystem_SetGravity(IntPtr system, ref JPH_Vec3 value);

        // Shapes
        [DllImport(LIB)] public static extern IntPtr JPH_BoxShape_Create(ref JPH_Vec3 halfExtent, float convexRadius);
        [DllImport(LIB)] public static extern IntPtr JPH_SphereShape_Create(float radius);
        [DllImport(LIB)] public static extern IntPtr JPH_CapsuleShape_Create(float halfHeightOfCylinder, float radius);
        [DllImport(LIB)] public static extern IntPtr JPH_CylinderShape_Create(float halfHeight, float radius);
        [DllImport(LIB)] public static extern IntPtr JPH_PlaneShape_Create(ref JPH_Plane plane, IntPtr material, float halfExtent);
        [DllImport(LIB)] public static extern void JPH_Shape_Destroy(IntPtr shape);

        // Body creation settings
        [DllImport(LIB)] public static extern IntPtr JPH_BodyCreationSettings_Create3(IntPtr shape, ref JPH_RVec3 position, ref JPH_Quat rotation, int motionType, uint objectLayer);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_Destroy(IntPtr settings);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_SetFriction(IntPtr settings, float value);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_SetRestitution(IntPtr settings, float value);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_SetLinearDamping(IntPtr settings, float value);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_SetAngularDamping(IntPtr settings, float value);
        [DllImport(LIB)] public static extern void JPH_BodyCreationSettings_SetGravityFactor(IntPtr settings, float value);

        // Body interface - create/destroy
        [DllImport(LIB)] public static extern uint JPH_BodyInterface_CreateAndAddBody(IntPtr bodyInterface, IntPtr settings, int activationMode);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_RemoveBody(IntPtr bodyInterface, uint bodyID);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_DestroyBody(IntPtr bodyInterface, uint bodyID);

        // Body interface - position/rotation
        [DllImport(LIB)] public static extern void JPH_BodyInterface_GetPosition(IntPtr bodyInterface, uint bodyId, out JPH_RVec3 result);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_GetRotation(IntPtr bodyInterface, uint bodyId, out JPH_Quat result);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_SetPosition(IntPtr bodyInterface, uint bodyId, ref JPH_RVec3 position, int activationMode);

        // Body interface - velocity
        [DllImport(LIB)] public static extern void JPH_BodyInterface_SetLinearVelocity(IntPtr bodyInterface, uint bodyID, ref JPH_Vec3 velocity);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_GetLinearVelocity(IntPtr bodyInterface, uint bodyID, out JPH_Vec3 velocity);

        // Body interface - forces
        [DllImport(LIB)] public static extern void JPH_BodyInterface_AddForce(IntPtr bodyInterface, uint bodyID, ref JPH_Vec3 force);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_AddImpulse(IntPtr bodyInterface, uint bodyID, ref JPH_Vec3 impulse);

        // Body interface - state
        [DllImport(LIB)] public static extern bool JPH_BodyInterface_IsActive(IntPtr bodyInterface, uint bodyID);
        [DllImport(LIB)] public static extern void JPH_BodyInterface_ActivateBody(IntPtr bodyInterface, uint bodyID);
    }
}

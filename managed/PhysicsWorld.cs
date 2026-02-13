using System;
using System.Collections.Generic;

namespace ECS
{
    public class PhysicsWorld
    {
        public static readonly PhysicsWorld Instance = new PhysicsWorld();

        const uint OBJ_LAYER_NON_MOVING = 0;
        const uint OBJ_LAYER_MOVING = 1;
        const uint NUM_OBJ_LAYERS = 2;
        const byte BP_LAYER_NON_MOVING = 0;
        const byte BP_LAYER_MOVING = 1;
        const uint NUM_BP_LAYERS = 2;

        const float FIXED_TIMESTEP = 1f / 60f;
        const int MAX_STEPS_PER_FRAME = 4;

        IntPtr jobSystem_;
        IntPtr physicsSystem_;
        IntPtr bodyInterface_;
        IntPtr objectLayerPairFilter_;
        IntPtr broadPhaseLayerInterface_;
        IntPtr objectVsBroadPhaseLayerFilter_;
        bool initialized_;
        float accumulator_;

        readonly Dictionary<int, uint> entityToBody_ = new Dictionary<int, uint>();

        PhysicsWorld() { }

        public void Init()
        {
            if (initialized_) return;

            if (!PhysicsBridge.JPH_Init())
            {
                Console.WriteLine("[PhysicsWorld] JPH_Init failed");
                return;
            }

            var jobConfig = new JobSystemThreadPoolConfig
            {
                maxJobs = 2048,
                maxBarriers = 8,
                numThreads = -1
            };
            jobSystem_ = PhysicsBridge.JPH_JobSystemThreadPool_Create(ref jobConfig);

            // Collision layers: MOVING objects collide with everything, NON_MOVING only with MOVING
            objectLayerPairFilter_ = PhysicsBridge.JPH_ObjectLayerPairFilterTable_Create(NUM_OBJ_LAYERS);
            PhysicsBridge.JPH_ObjectLayerPairFilterTable_EnableCollision(objectLayerPairFilter_, OBJ_LAYER_NON_MOVING, OBJ_LAYER_MOVING);
            PhysicsBridge.JPH_ObjectLayerPairFilterTable_EnableCollision(objectLayerPairFilter_, OBJ_LAYER_MOVING, OBJ_LAYER_MOVING);

            broadPhaseLayerInterface_ = PhysicsBridge.JPH_BroadPhaseLayerInterfaceTable_Create(NUM_OBJ_LAYERS, NUM_BP_LAYERS);
            PhysicsBridge.JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(broadPhaseLayerInterface_, OBJ_LAYER_NON_MOVING, BP_LAYER_NON_MOVING);
            PhysicsBridge.JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(broadPhaseLayerInterface_, OBJ_LAYER_MOVING, BP_LAYER_MOVING);

            objectVsBroadPhaseLayerFilter_ = PhysicsBridge.JPH_ObjectVsBroadPhaseLayerFilterTable_Create(
                broadPhaseLayerInterface_, NUM_BP_LAYERS, objectLayerPairFilter_, NUM_OBJ_LAYERS);

            var settings = new JPH_PhysicsSystemSettings
            {
                maxBodies = 4096,
                numBodyMutexes = 0,
                maxBodyPairs = 4096,
                maxContactConstraints = 4096,
                _padding = 0,
                broadPhaseLayerInterface = broadPhaseLayerInterface_,
                objectLayerPairFilter = objectLayerPairFilter_,
                objectVsBroadPhaseLayerFilter = objectVsBroadPhaseLayerFilter_
            };
            physicsSystem_ = PhysicsBridge.JPH_PhysicsSystem_Create(ref settings);

            var gravity = new JPH_Vec3(0f, -9.81f, 0f);
            PhysicsBridge.JPH_PhysicsSystem_SetGravity(physicsSystem_, ref gravity);

            bodyInterface_ = PhysicsBridge.JPH_PhysicsSystem_GetBodyInterface(physicsSystem_);

            accumulator_ = 0f;
            initialized_ = true;
            Console.WriteLine("[PhysicsWorld] Initialized");
        }

        public void Step(float frameDeltaTime)
        {
            if (!initialized_) return;

            accumulator_ += frameDeltaTime;
            int steps = 0;
            while (accumulator_ >= FIXED_TIMESTEP && steps < MAX_STEPS_PER_FRAME)
            {
                PhysicsBridge.JPH_PhysicsSystem_Update(physicsSystem_, FIXED_TIMESTEP, 1, jobSystem_);
                accumulator_ -= FIXED_TIMESTEP;
                steps++;
            }
            if (accumulator_ > FIXED_TIMESTEP * MAX_STEPS_PER_FRAME)
                accumulator_ = 0f;
        }

        public uint CreateBody(int entityId, IntPtr shape, float posX, float posY, float posZ,
                               JPH_Quat rotation, JPH_MotionType motionType,
                               float friction, float restitution, float linearDamping,
                               float angularDamping, float gravityFactor)
        {
            if (!initialized_) return 0;

            uint objLayer = (motionType == JPH_MotionType.Static) ? OBJ_LAYER_NON_MOVING : OBJ_LAYER_MOVING;
            var pos = new JPH_RVec3(posX, posY, posZ);
            IntPtr bodySettings = PhysicsBridge.JPH_BodyCreationSettings_Create3(
                shape, ref pos, ref rotation, (int)motionType, objLayer);

            PhysicsBridge.JPH_BodyCreationSettings_SetFriction(bodySettings, friction);
            PhysicsBridge.JPH_BodyCreationSettings_SetRestitution(bodySettings, restitution);
            PhysicsBridge.JPH_BodyCreationSettings_SetLinearDamping(bodySettings, linearDamping);
            PhysicsBridge.JPH_BodyCreationSettings_SetAngularDamping(bodySettings, angularDamping);
            PhysicsBridge.JPH_BodyCreationSettings_SetGravityFactor(bodySettings, gravityFactor);

            uint bodyId = PhysicsBridge.JPH_BodyInterface_CreateAndAddBody(
                bodyInterface_, bodySettings, (int)JPH_Activation.Activate);

            PhysicsBridge.JPH_BodyCreationSettings_Destroy(bodySettings);

            entityToBody_[entityId] = bodyId;
            return bodyId;
        }

        public void RemoveBody(int entityId)
        {
            if (!initialized_) return;
            uint bodyId;
            if (!entityToBody_.TryGetValue(entityId, out bodyId)) return;

            PhysicsBridge.JPH_BodyInterface_RemoveBody(bodyInterface_, bodyId);
            PhysicsBridge.JPH_BodyInterface_DestroyBody(bodyInterface_, bodyId);
            entityToBody_.Remove(entityId);
        }

        public void RemoveAllBodies()
        {
            if (!initialized_) return;

            foreach (var kvp in entityToBody_)
            {
                PhysicsBridge.JPH_BodyInterface_RemoveBody(bodyInterface_, kvp.Value);
                PhysicsBridge.JPH_BodyInterface_DestroyBody(bodyInterface_, kvp.Value);
            }
            entityToBody_.Clear();
        }

        public void GetBodyPosition(uint bodyId, out float x, out float y, out float z)
        {
            JPH_RVec3 pos;
            PhysicsBridge.JPH_BodyInterface_GetPosition(bodyInterface_, bodyId, out pos);
            x = (float)pos.x;
            y = (float)pos.y;
            z = (float)pos.z;
        }

        public void GetBodyRotation(uint bodyId, out JPH_Quat rotation)
        {
            PhysicsBridge.JPH_BodyInterface_GetRotation(bodyInterface_, bodyId, out rotation);
        }

        public bool IsBodyActive(uint bodyId)
        {
            if (!initialized_) return false;
            return PhysicsBridge.JPH_BodyInterface_IsActive(bodyInterface_, bodyId);
        }

        public void OptimizeBroadPhase()
        {
            if (!initialized_) return;
            PhysicsBridge.JPH_PhysicsSystem_OptimizeBroadPhase(physicsSystem_);
        }

        public void Shutdown()
        {
            if (!initialized_) return;

            RemoveAllBodies();

            if (physicsSystem_ != IntPtr.Zero)
            {
                PhysicsBridge.JPH_PhysicsSystem_Destroy(physicsSystem_);
                physicsSystem_ = IntPtr.Zero;
            }
            if (jobSystem_ != IntPtr.Zero)
            {
                PhysicsBridge.JPH_JobSystem_Destroy(jobSystem_);
                jobSystem_ = IntPtr.Zero;
            }

            PhysicsBridge.JPH_Shutdown();

            bodyInterface_ = IntPtr.Zero;
            objectLayerPairFilter_ = IntPtr.Zero;
            broadPhaseLayerInterface_ = IntPtr.Zero;
            objectVsBroadPhaseLayerFilter_ = IntPtr.Zero;

            initialized_ = false;
            Console.WriteLine("[PhysicsWorld] Shut down");
        }
    }
}

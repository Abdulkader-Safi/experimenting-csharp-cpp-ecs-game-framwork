using System;
using System.Collections.Generic;

namespace ECS
{
    public static class Systems
    {
        const float MAX_PITCH = 89f;

        private static float ClampPitch(float pitch)
        {
            if (pitch > MAX_PITCH) return MAX_PITCH;
            if (pitch < -MAX_PITCH) return -MAX_PITCH;
            return pitch;
        }

        public static void InputMovementSystem(World world)
        {
            if (FreeCameraState.IsActive) return;

            List<int> entities = world.Query(typeof(Movable), typeof(Transform));
            foreach (int e in entities)
            {
                var mov = world.GetComponent<Movable>(e);
                var tr = world.GetComponent<Transform>(e);

                float moveStep = mov.Speed * world.DeltaTime;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_LEFT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_A))
                    tr.Rotation.Y -= moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_RIGHT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_D))
                    tr.Rotation.Y += moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_UP) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W))
                    tr.Rotation.X -= moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_DOWN) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_S))
                    tr.Rotation.X += moveStep;
            }
        }

        public static void TimerSystem(World world)
        {
            List<int> entities = world.Query(typeof(Timer));
            foreach (int e in entities)
            {
                var timer = world.GetComponent<Timer>(e);
                if (timer.Finished && !timer.Repeat) continue;

                timer.Elapsed += world.DeltaTime;
                if (timer.Elapsed >= timer.Duration)
                {
                    timer.Finished = true;
                    if (timer.Repeat)
                        timer.Elapsed -= timer.Duration;
                }
            }
        }

        public static void FreeCameraSystem(World world)
        {
            // Key 0: activate free camera (edge-detected)
            bool key0 = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_0);
            if (key0 && !FreeCameraState.WasKey0Pressed && GameConstants.Debug)
            {
                FreeCameraState.IsActive = true;
                FreeCameraState.MouseInitialized = false;
            }
            FreeCameraState.WasKey0Pressed = key0;

            // Key 1: deactivate free camera (edge-detected)
            bool key1 = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_1);
            if (key1 && !FreeCameraState.WasKey1Pressed)
            {
                if (FreeCameraState.IsActive)
                {
                    FreeCameraState.IsActive = false;
                    // Reset entity camera mouse state to avoid delta jump on switch back
                    List<int> camEntities = world.Query(typeof(Camera));
                    foreach (int e in camEntities)
                    {
                        var cam = world.GetComponent<Camera>(e);
                        cam._MouseInitialized = false;
                    }
                    NativeBridge.ResetScrollOffset();
                }
            }
            FreeCameraState.WasKey1Pressed = key1;

            if (!FreeCameraState.IsActive) return;

            float dt = world.DeltaTime;

            // ESC toggle cursor lock (edge-detected, reuse entity camera's _WasEscPressed for state)
            bool escPressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_ESCAPE);
            List<int> cameraEntities = world.Query(typeof(Camera));
            foreach (int e in cameraEntities)
            {
                var cam = world.GetComponent<Camera>(e);
                if (escPressed && !cam._WasEscPressed)
                {
                    bool locked = NativeBridge.IsCursorLocked();
                    NativeBridge.SetCursorLocked(!locked);
                    FreeCameraState.MouseInitialized = false;
                }
                cam._WasEscPressed = escPressed;
            }

            // Mouse look when cursor is locked
            if (NativeBridge.IsCursorLocked())
            {
                double mx, my;
                NativeBridge.GetCursorPos(out mx, out my);

                if (!FreeCameraState.MouseInitialized)
                {
                    FreeCameraState.LastMouseX = mx;
                    FreeCameraState.LastMouseY = my;
                    FreeCameraState.MouseInitialized = true;
                }
                else
                {
                    double dx = mx - FreeCameraState.LastMouseX;
                    double dy = my - FreeCameraState.LastMouseY;
                    FreeCameraState.Yaw += (float)dx * GameConstants.FreeCamSensitivity;
                    FreeCameraState.Pitch -= (float)dy * GameConstants.FreeCamSensitivity;
                }

                FreeCameraState.LastMouseX = mx;
                FreeCameraState.LastMouseY = my;
            }

            FreeCameraState.Pitch = ClampPitch(FreeCameraState.Pitch);

            double yawRad = FreeCameraState.Yaw * Transform.DegToRad;
            double pitchRad = FreeCameraState.Pitch * Transform.DegToRad;

            // Forward and right vectors from yaw/pitch (yaw=0 faces -Z)
            float fwdX = (float)(Math.Cos(pitchRad) * Math.Sin(yawRad));
            float fwdY = (float)Math.Sin(pitchRad);
            float fwdZ = -(float)(Math.Cos(pitchRad) * Math.Cos(yawRad));

            // Right vector (perpendicular to forward on XZ plane)
            float rightX = (float)Math.Cos(yawRad);
            float rightZ = (float)Math.Sin(yawRad);

            float speed = GameConstants.FreeCamSpeed * dt;

            // WASD movement
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W))
            {
                FreeCameraState.Position.X += fwdX * speed;
                FreeCameraState.Position.Y += fwdY * speed;
                FreeCameraState.Position.Z += fwdZ * speed;
            }
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_S))
            {
                FreeCameraState.Position.X -= fwdX * speed;
                FreeCameraState.Position.Y -= fwdY * speed;
                FreeCameraState.Position.Z -= fwdZ * speed;
            }
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_A))
            {
                FreeCameraState.Position.X -= rightX * speed;
                FreeCameraState.Position.Z -= rightZ * speed;
            }
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_D))
            {
                FreeCameraState.Position.X += rightX * speed;
                FreeCameraState.Position.Z += rightZ * speed;
            }

            // Q down, E up
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_Q))
                FreeCameraState.Position.Y -= speed;
            if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_E))
                FreeCameraState.Position.Y += speed;

            // Set camera: look-at = position + forward direction
            NativeBridge.SetCamera(
                FreeCameraState.Position.X, FreeCameraState.Position.Y, FreeCameraState.Position.Z,
                FreeCameraState.Position.X + fwdX, FreeCameraState.Position.Y + fwdY, FreeCameraState.Position.Z + fwdZ,
                0f, 1f, 0f, FreeCameraState.Fov);
        }

        public static void CameraFollowSystem(World world)
        {
            if (FreeCameraState.IsActive) return;

            List<int> entities = world.Query(typeof(Camera), typeof(Transform));
            foreach (int e in entities)
            {
                var cam = world.GetComponent<Camera>(e);
                var tr = world.GetComponent<Transform>(e);

                // ESC toggle cursor lock (edge-detected)
                bool escPressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_ESCAPE);
                if (escPressed && !cam._WasEscPressed)
                {
                    bool locked = NativeBridge.IsCursorLocked();
                    NativeBridge.SetCursorLocked(!locked);
                    cam._MouseInitialized = false;
                }
                cam._WasEscPressed = escPressed;

                // TAB toggle camera mode (edge-detected)
                bool tabPressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_TAB);
                if (tabPressed && !cam._WasModeTogglePressed)
                {
                    cam.Mode = (cam.Mode == CameraMode.ThirdPerson) ? CameraMode.FirstPerson : CameraMode.ThirdPerson;
                }
                cam._WasModeTogglePressed = tabPressed;

                // Mouse look when cursor is locked
                if (NativeBridge.IsCursorLocked())
                {
                    double mx, my;
                    NativeBridge.GetCursorPos(out mx, out my);

                    if (!cam._MouseInitialized)
                    {
                        cam._LastMouseX = mx;
                        cam._LastMouseY = my;
                        cam._MouseInitialized = true;
                    }
                    else
                    {
                        double dx = mx - cam._LastMouseX;
                        double dy = my - cam._LastMouseY;
                        cam.Yaw -= (float)dx * cam.MouseSensitivity;
                        cam.Pitch += (float)dy * cam.MouseSensitivity;
                    }

                    cam._LastMouseX = mx;
                    cam._LastMouseY = my;
                }

                // Q/E orbit yaw, R/F orbit pitch (always works)
                float lookStep = cam.LookSpeed * world.DeltaTime;
                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_Q))
                    cam.Yaw -= lookStep;
                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_E))
                    cam.Yaw += lookStep;
                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_R))
                    cam.Pitch += lookStep;
                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_F))
                    cam.Pitch -= lookStep;

                cam.Pitch = ClampPitch(cam.Pitch);

                double yawRad = cam.Yaw * Transform.DegToRad;
                double pitchRad = cam.Pitch * Transform.DegToRad;

                if (cam.Mode == CameraMode.FirstPerson)
                {
                    // First-person: eye at entity position + eye height, look in yaw/pitch direction
                    float eyeX = tr.Position.X;
                    float eyeY = tr.Position.Y + cam.EyeHeight;
                    float eyeZ = tr.Position.Z;

                    float dirX = (float)(Math.Cos(pitchRad) * Math.Sin(yawRad));
                    float dirY = (float)Math.Sin(pitchRad);
                    float dirZ = (float)(Math.Cos(pitchRad) * Math.Cos(yawRad));

                    NativeBridge.SetCamera(eyeX, eyeY, eyeZ,
                                           eyeX + dirX, eyeY + dirY, eyeZ + dirZ,
                                           0f, 1f, 0f, cam.Fov);
                }
                else
                {
                    // Third-person orbit: scroll wheel zooms
                    float scrollX, scrollY;
                    NativeBridge.GetScrollOffset(out scrollX, out scrollY);
                    NativeBridge.ResetScrollOffset();

                    float dist = (float)Math.Sqrt(cam.Offset.X * cam.Offset.X +
                                                   cam.Offset.Y * cam.Offset.Y +
                                                   cam.Offset.Z * cam.Offset.Z);
                    dist -= scrollY * cam.ZoomSpeed;
                    if (dist < cam.MinDistance) dist = cam.MinDistance;
                    if (dist > cam.MaxDistance) dist = cam.MaxDistance;

                    // Store back into offset (along Z for simplicity)
                    cam.Offset.X = 0f;
                    cam.Offset.Y = 0f;
                    cam.Offset.Z = dist;

                    float eyeX = tr.Position.X + dist * (float)(Math.Cos(pitchRad) * Math.Sin(yawRad));
                    float eyeY = tr.Position.Y + dist * (float)Math.Sin(pitchRad);
                    float eyeZ = tr.Position.Z + dist * (float)(Math.Cos(pitchRad) * Math.Cos(yawRad));

                    NativeBridge.SetCamera(eyeX, eyeY, eyeZ,
                                           tr.Position.X, tr.Position.Y, tr.Position.Z,
                                           0f, 1f, 0f, cam.Fov);
                }
            }
        }

        public static void LightSyncSystem(World world)
        {
            List<int> entities = world.Query(typeof(Light), typeof(Transform));
            int slot = 0;
            foreach (int e in entities)
            {
                if (slot >= 8) break;
                var light = world.GetComponent<Light>(e);
                var tr = world.GetComponent<Transform>(e);

                light._LightIndex = slot;

                float innerCos = (float)Math.Cos(light.InnerConeDeg * Transform.DegToRad);
                float outerCos = (float)Math.Cos(light.OuterConeDeg * Transform.DegToRad);

                NativeBridge.SetLight(slot, (int)light.Type,
                    tr.Position.X, tr.Position.Y, tr.Position.Z,
                    light.Direction.X, light.Direction.Y, light.Direction.Z,
                    light.LightColor.R, light.LightColor.G, light.LightColor.B, light.Intensity,
                    light.Radius, innerCos, outerCos);
                slot++;
            }

            // Clear unused slots
            for (int i = slot; i < 8; i++)
                NativeBridge.ClearLight(i);
        }

        public static void HierarchyTransformSystem(World world)
        {
            List<int> entities = world.Query(typeof(Transform));
            foreach (int e in entities)
            {
                var tr = world.GetComponent<Transform>(e);
                var hier = world.GetComponent<Hierarchy>(e);

                float[] localMatrix = tr.ToMatrix();

                if (hier != null && hier.Parent >= 0 && world.IsAlive(hier.Parent))
                {
                    // Walk parent chain to get parent's world transform
                    float[] parentWorld = GetWorldMatrix(world, hier.Parent);
                    float[] worldMatrix = Transform.MultiplyMatrices(parentWorld, localMatrix);

                    var wt = world.GetComponent<WorldTransform>(e);
                    if (wt == null)
                    {
                        wt = new WorldTransform();
                        world.AddComponent(e, wt);
                    }
                    wt.Matrix = worldMatrix;
                }
                else
                {
                    // Root entity — world transform is local transform
                    var wt = world.GetComponent<WorldTransform>(e);
                    if (wt != null)
                    {
                        wt.Matrix = localMatrix;
                    }
                }
            }
        }

        private static float[] GetWorldMatrix(World world, int entity)
        {
            var tr = world.GetComponent<Transform>(entity);
            if (tr == null)
                return (float[])WorldTransform.Identity.Clone();

            float[] localMatrix = tr.ToMatrix();
            var hier = world.GetComponent<Hierarchy>(entity);

            if (hier != null && hier.Parent >= 0 && world.IsAlive(hier.Parent))
            {
                float[] parentWorld = GetWorldMatrix(world, hier.Parent);
                return Transform.MultiplyMatrices(parentWorld, localMatrix);
            }

            return localMatrix;
        }

        private static bool wasF3Pressed_ = false;

        public static void DebugOverlaySystem(World world)
        {
            bool f3 = NativeBridge.IsKeyPressed(GameConstants.GLFW_KEY_F3);
            if (f3 && !wasF3Pressed_)
            {
                GameConstants.Debug = !GameConstants.Debug;
            }
            wasF3Pressed_ = f3;

            NativeBridge.SetDebugOverlay(GameConstants.Debug);
        }

        public static void PhysicsSystem(World world)
        {
            // Phase 1: Create bodies for new Rigidbody+Collider+Transform entities
            List<int> physEntities = world.Query(typeof(Rigidbody), typeof(Collider), typeof(Transform));
            foreach (int e in physEntities)
            {
                var rb = world.GetComponent<Rigidbody>(e);
                if (rb._BodyCreated) continue;

                var col = world.GetComponent<Collider>(e);
                var tr = world.GetComponent<Transform>(e);

                IntPtr shape = CreateShape(col);
                if (shape == IntPtr.Zero) continue;

                rb._BodyId = PhysicsWorld.Instance.CreateBody(
                    e, shape, tr.Position.X, tr.Position.Y, tr.Position.Z,
                    JPH_Quat.Identity, rb.MotionType,
                    rb.Friction, rb.Restitution, rb.LinearDamping,
                    rb.AngularDamping, rb.GravityFactor);
                rb._BodyCreated = true;
            }

            // Phase 2: Step physics simulation
            PhysicsWorld.Instance.Step(world.DeltaTime);

            // Phase 3: Sync transforms back from Jolt for dynamic bodies
            foreach (int e in physEntities)
            {
                var rb = world.GetComponent<Rigidbody>(e);
                if (!rb._BodyCreated || rb.MotionType == JPH_MotionType.Static) continue;
                if (!PhysicsWorld.Instance.IsBodyActive(rb._BodyId)) continue;

                var tr = world.GetComponent<Transform>(e);

                float px, py, pz;
                PhysicsWorld.Instance.GetBodyPosition(rb._BodyId, out px, out py, out pz);
                tr.Position.X = px;
                tr.Position.Y = py;
                tr.Position.Z = pz;

                JPH_Quat rot;
                PhysicsWorld.Instance.GetBodyRotation(rb._BodyId, out rot);
                QuatToEulerDeg(rot, out tr.Rotation.X, out tr.Rotation.Y, out tr.Rotation.Z);
            }
        }

        private static IntPtr CreateShape(Collider col)
        {
            switch (col.Shape)
            {
                case ShapeType.Box:
                    var halfExt = new JPH_Vec3(col.BoxHalfExtents.X, col.BoxHalfExtents.Y, col.BoxHalfExtents.Z);
                    return PhysicsBridge.JPH_BoxShape_Create(ref halfExt, 0.05f);
                case ShapeType.Sphere:
                    return PhysicsBridge.JPH_SphereShape_Create(col.SphereRadius);
                case ShapeType.Capsule:
                    return PhysicsBridge.JPH_CapsuleShape_Create(col.CapsuleHalfHeight, col.CapsuleRadius);
                case ShapeType.Cylinder:
                    return PhysicsBridge.JPH_CylinderShape_Create(col.CylinderHalfHeight, col.CylinderRadius);
                case ShapeType.Plane:
                    var normal = new JPH_Vec3(col.PlaneNormal.X, col.PlaneNormal.Y, col.PlaneNormal.Z);
                    var plane = new JPH_Plane(normal, col.PlaneDistance);
                    return PhysicsBridge.JPH_PlaneShape_Create(ref plane, IntPtr.Zero, col.PlaneHalfExtent);
                default:
                    return IntPtr.Zero;
            }
        }

        private static void QuatToEulerDeg(JPH_Quat q, out float rx, out float ry, out float rz)
        {
            const double RadToDeg = 180.0 / Math.PI;

            // Roll (X)
            double sinr = 2.0 * (q.w * q.x + q.y * q.z);
            double cosr = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
            rx = (float)(Math.Atan2(sinr, cosr) * RadToDeg);

            // Pitch (Y)
            double sinp = 2.0 * (q.w * q.y - q.z * q.x);
            if (Math.Abs(sinp) >= 1.0)
                ry = (float)(Math.Sign(sinp) * 90.0);
            else
                ry = (float)(Math.Asin(sinp) * RadToDeg);

            // Yaw (Z)
            double siny = 2.0 * (q.w * q.z + q.x * q.y);
            double cosy = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
            rz = (float)(Math.Atan2(siny, cosy) * RadToDeg);
        }

        // Debug collider visualization — tracks debug renderer entity IDs per ECS entity
        private static Dictionary<int, int> debugColliderEntities_ = new Dictionary<int, int>();
        private static Dictionary<int, int> debugColliderMeshes_ = new Dictionary<int, int>();
        private static bool wasDebugOn_ = false;

        public static void DebugColliderRenderSystem(World world)
        {
            bool debugOn = GameConstants.Debug;

            // Turning off: clear all debug entities
            if (!debugOn && wasDebugOn_)
            {
                NativeBridge.ClearDebugEntities();
                debugColliderEntities_.Clear();
                debugColliderMeshes_.Clear();
            }
            wasDebugOn_ = debugOn;

            if (!debugOn) return;

            List<int> entities = world.Query(typeof(Collider), typeof(Transform));

            // Remove debug entities for ECS entities that no longer exist
            List<int> toRemove = new List<int>();
            foreach (var kvp in debugColliderEntities_)
            {
                if (!world.IsAlive(kvp.Key))
                    toRemove.Add(kvp.Key);
            }
            foreach (int id in toRemove)
            {
                NativeBridge.RemoveDebugEntity(debugColliderEntities_[id]);
                debugColliderEntities_.Remove(id);
                debugColliderMeshes_.Remove(id);
            }

            foreach (int e in entities)
            {
                var col = world.GetComponent<Collider>(e);
                var tr = world.GetComponent<Transform>(e);

                // Skip planes (too large to render meaningfully)
                if (col.Shape == ShapeType.Plane) continue;

                // Create debug entity if not tracked
                if (!debugColliderEntities_.ContainsKey(e))
                {
                    int meshId = CreateColliderMesh(col, col.DebugColor);
                    if (meshId < 0) continue;

                    int debugId = NativeBridge.CreateDebugEntity(meshId);
                    if (debugId < 0) continue;

                    debugColliderEntities_[e] = debugId;
                    debugColliderMeshes_[e] = meshId;
                }

                // Compute transform: position from Transform, scale from collider dimensions
                float[] matrix = tr.ToMatrix();
                NativeBridge.SetDebugEntityTransform(debugColliderEntities_[e], matrix);
            }
        }

        private static int CreateColliderMesh(Collider col, Color c)
        {
            switch (col.Shape)
            {
                case ShapeType.Box:
                    return NativeBridge.CreateBoxMesh(
                        col.BoxHalfExtents.X * 2f, col.BoxHalfExtents.Y * 2f, col.BoxHalfExtents.Z * 2f,
                        c.R, c.G, c.B);
                case ShapeType.Sphere:
                    return NativeBridge.CreateSphereMesh(
                        col.SphereRadius, 16, 8, c.R, c.G, c.B);
                case ShapeType.Capsule:
                    return NativeBridge.CreateCapsuleMesh(
                        col.CapsuleRadius, col.CapsuleHalfHeight * 2f, 16, 8,
                        c.R, c.G, c.B);
                case ShapeType.Cylinder:
                    return NativeBridge.CreateCylinderMesh(
                        col.CylinderRadius, col.CylinderHalfHeight * 2f, 16,
                        c.R, c.G, c.B);
                default:
                    return -1;
            }
        }

        public static void RenderSyncSystem(World world)
        {
            List<int> entities = world.Query(typeof(Transform), typeof(MeshComponent));
            foreach (int e in entities)
            {
                var tr = world.GetComponent<Transform>(e);
                var mc = world.GetComponent<MeshComponent>(e);

                if (mc._RendererEntityId >= 0)
                {
                    var wt = world.GetComponent<WorldTransform>(e);
                    float[] matrix = (wt != null) ? wt.Matrix : tr.ToMatrix();
                    NativeBridge.SetEntityTransform(mc._RendererEntityId, matrix);
                }
            }
        }
    }
}

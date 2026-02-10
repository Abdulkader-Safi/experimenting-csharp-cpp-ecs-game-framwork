using System;
using System.Collections.Generic;

namespace ECS
{
    public static class Systems
    {
        public static void InputMovementSystem(World world)
        {
            List<int> entities = world.Query(typeof(Movable), typeof(Transform));
            foreach (int e in entities)
            {
                var mov = world.GetComponent<Movable>(e);
                var tr = world.GetComponent<Transform>(e);

                float moveStep = mov.Speed * world.DeltaTime;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_LEFT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_A))
                    tr.RotY -= moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_RIGHT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_D))
                    tr.RotY += moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_UP) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W))
                    tr.RotX -= moveStep;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_DOWN) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_S))
                    tr.RotX += moveStep;
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

        public static void CameraFollowSystem(World world)
        {
            List<int> entities = world.Query(typeof(Camera), typeof(Transform));
            foreach (int e in entities)
            {
                var cam = world.GetComponent<Camera>(e);
                var tr = world.GetComponent<Transform>(e);

                // ESC toggle cursor lock (edge-detected)
                bool escPressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_ESCAPE);
                if (escPressed && !cam.WasEscPressed)
                {
                    bool locked = NativeBridge.IsCursorLocked();
                    NativeBridge.SetCursorLocked(!locked);
                    cam.MouseInitialized = false;
                }
                cam.WasEscPressed = escPressed;

                // TAB toggle camera mode (edge-detected)
                bool tabPressed = NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_TAB);
                if (tabPressed && !cam.WasModeTogglePressed)
                {
                    cam.Mode = (cam.Mode == 0) ? 1 : 0;
                }
                cam.WasModeTogglePressed = tabPressed;

                // Mouse look when cursor is locked
                if (NativeBridge.IsCursorLocked())
                {
                    double mx, my;
                    NativeBridge.GetCursorPos(out mx, out my);

                    if (!cam.MouseInitialized)
                    {
                        cam.LastMouseX = mx;
                        cam.LastMouseY = my;
                        cam.MouseInitialized = true;
                    }
                    else
                    {
                        double dx = mx - cam.LastMouseX;
                        double dy = my - cam.LastMouseY;
                        cam.Yaw -= (float)dx * cam.MouseSensitivity;
                        cam.Pitch += (float)dy * cam.MouseSensitivity;
                    }

                    cam.LastMouseX = mx;
                    cam.LastMouseY = my;
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

                // Clamp pitch
                if (cam.Pitch > 89f) cam.Pitch = 89f;
                if (cam.Pitch < -89f) cam.Pitch = -89f;

                double yawRad = cam.Yaw * Math.PI / 180.0;
                double pitchRad = cam.Pitch * Math.PI / 180.0;

                if (cam.Mode == 1)
                {
                    // First-person: eye at entity position + eye height, look in yaw/pitch direction
                    float eyeX = tr.X;
                    float eyeY = tr.Y + cam.EyeHeight;
                    float eyeZ = tr.Z;

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

                    float dist = (float)Math.Sqrt(cam.OffsetX * cam.OffsetX +
                                                   cam.OffsetY * cam.OffsetY +
                                                   cam.OffsetZ * cam.OffsetZ);
                    dist -= scrollY * cam.ZoomSpeed;
                    if (dist < cam.MinDistance) dist = cam.MinDistance;
                    if (dist > cam.MaxDistance) dist = cam.MaxDistance;

                    // Store back into offset (along Z for simplicity)
                    cam.OffsetX = 0f;
                    cam.OffsetY = 0f;
                    cam.OffsetZ = dist;

                    float eyeX = tr.X + dist * (float)(Math.Cos(pitchRad) * Math.Sin(yawRad));
                    float eyeY = tr.Y + dist * (float)Math.Sin(pitchRad);
                    float eyeZ = tr.Z + dist * (float)(Math.Cos(pitchRad) * Math.Cos(yawRad));

                    NativeBridge.SetCamera(eyeX, eyeY, eyeZ,
                                           tr.X, tr.Y, tr.Z,
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

                light.LightIndex = slot;

                float innerCos = (float)Math.Cos(light.InnerConeDeg * Math.PI / 180.0);
                float outerCos = (float)Math.Cos(light.OuterConeDeg * Math.PI / 180.0);

                NativeBridge.SetLight(slot, light.Type,
                    tr.X, tr.Y, tr.Z,
                    light.DirX, light.DirY, light.DirZ,
                    light.ColorR, light.ColorG, light.ColorB, light.Intensity,
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
                return new float[] { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

            float[] localMatrix = tr.ToMatrix();
            var hier = world.GetComponent<Hierarchy>(entity);

            if (hier != null && hier.Parent >= 0 && world.IsAlive(hier.Parent))
            {
                float[] parentWorld = GetWorldMatrix(world, hier.Parent);
                return Transform.MultiplyMatrices(parentWorld, localMatrix);
            }

            return localMatrix;
        }

        public static void RenderSyncSystem(World world)
        {
            List<int> entities = world.Query(typeof(Transform), typeof(MeshComponent));
            foreach (int e in entities)
            {
                var tr = world.GetComponent<Transform>(e);
                var mc = world.GetComponent<MeshComponent>(e);

                if (mc.RendererEntityId >= 0)
                {
                    var wt = world.GetComponent<WorldTransform>(e);
                    float[] matrix = (wt != null) ? wt.Matrix : tr.ToMatrix();
                    NativeBridge.SetEntityTransform(mc.RendererEntityId, matrix);
                }
            }
        }
    }
}

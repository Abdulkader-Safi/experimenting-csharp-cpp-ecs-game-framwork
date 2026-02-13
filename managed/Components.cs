using System;

namespace ECS
{
    public class Transform
    {
        public float X = 0f, Y = 0f, Z = 0f;
        public float RotX = 0f, RotY = 0f, RotZ = 0f;
        public float ScaleX = 1f, ScaleY = 1f, ScaleZ = 1f;

        // Returns a column-major 4x4 matrix (matching GLM layout)
        // Order: Scale -> RotX -> RotY -> RotZ -> Translate
        public float[] ToMatrix()
        {
            float cx = (float)Math.Cos(RotX * Math.PI / 180.0);
            float sx = (float)Math.Sin(RotX * Math.PI / 180.0);
            float cy = (float)Math.Cos(RotY * Math.PI / 180.0);
            float sy = (float)Math.Sin(RotY * Math.PI / 180.0);
            float cz = (float)Math.Cos(RotZ * Math.PI / 180.0);
            float sz = (float)Math.Sin(RotZ * Math.PI / 180.0);

            // Combined rotation: Rz * Ry * Rx (same order as glm::rotate chain)
            float r00 = cy * cz;
            float r01 = cz * sx * sy - cx * sz;
            float r02 = sx * sz + cx * cz * sy;

            float r10 = cy * sz;
            float r11 = cx * cz + sx * sy * sz;
            float r12 = cx * sy * sz - cz * sx;

            float r20 = -sy;
            float r21 = cy * sx;
            float r22 = cx * cy;

            // Column-major: mat[col*4 + row]
            return new float[] {
                r00 * ScaleX, r10 * ScaleX, r20 * ScaleX, 0f,  // column 0
                r01 * ScaleY, r11 * ScaleY, r21 * ScaleY, 0f,  // column 1
                r02 * ScaleZ, r12 * ScaleZ, r22 * ScaleZ, 0f,  // column 2
                X,            Y,            Z,            1f   // column 3
            };
        }

        // Column-major 4x4 matrix multiply: result = a * b
        public static float[] MultiplyMatrices(float[] a, float[] b)
        {
            float[] r = new float[16];
            for (int col = 0; col < 4; col++)
            {
                for (int row = 0; row < 4; row++)
                {
                    float sum = 0f;
                    for (int k = 0; k < 4; k++)
                    {
                        sum += a[k * 4 + row] * b[col * 4 + k];
                    }
                    r[col * 4 + row] = sum;
                }
            }
            return r;
        }
    }

    public class Hierarchy
    {
        public int Parent = -1;  // -1 = root (no parent)
    }

    public class WorldTransform
    {
        public float[] Matrix = new float[] {
            1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
        };
    }

    public class MeshComponent
    {
        public int MeshId = -1;
        public int RendererEntityId = -1;
    }

    public class Movable
    {
        public float Speed = 90f;
    }

    public class Light
    {
        public const int Directional = 0, Point = 1, Spot = 2;
        public int Type = Directional;
        public float ColorR = 1f, ColorG = 1f, ColorB = 1f;
        public float Intensity = 1f;
        public float DirX = 0f, DirY = -1f, DirZ = 0f;
        public float Radius = 10f;
        public float InnerConeDeg = 12.5f, OuterConeDeg = 17.5f;
        public int LightIndex = -1;
    }

    public class Timer
    {
        public float Duration = 1f;
        public float Elapsed = 0f;
        public bool Repeat = false;
        public bool Finished = false;
        public string Tag = "";
    }

    public class Camera
    {
        public float OffsetX = 0f, OffsetY = 0f, OffsetZ = 3f;
        public float Yaw = 0f;
        public float Pitch = 0f;
        public float Fov = 45f;
        public float LookSpeed = 90f;
        public float MouseSensitivity = 0.15f;
        public double LastMouseX = 0.0, LastMouseY = 0.0;
        public bool MouseInitialized = false;
        public bool WasEscPressed = false;

        // Camera mode: 0 = third-person orbit, 1 = first-person
        public int Mode = 0;
        public float EyeHeight = 0.8f;
        public bool WasModeTogglePressed = false;
        public float MinDistance = 1f;
        public float MaxDistance = 20f;
        public float ZoomSpeed = 2f;
    }
}

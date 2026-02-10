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
    }
}

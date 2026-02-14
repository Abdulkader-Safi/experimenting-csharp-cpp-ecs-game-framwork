using System;

namespace ECS
{
    public class Color
    {
        public float R, G, B, A;

        public Color(float r, float g, float b, float a = 1f) { R = r; G = g; B = b; A = a; }

        public Color(string hex)
        {
            if (hex.StartsWith("#")) hex = hex.Substring(1);
            R = int.Parse(hex.Substring(0, 2), System.Globalization.NumberStyles.HexNumber) / 255f;
            G = int.Parse(hex.Substring(2, 2), System.Globalization.NumberStyles.HexNumber) / 255f;
            B = int.Parse(hex.Substring(4, 2), System.Globalization.NumberStyles.HexNumber) / 255f;
            A = hex.Length >= 8
                ? int.Parse(hex.Substring(6, 2), System.Globalization.NumberStyles.HexNumber) / 255f
                : 1f;
        }

        public static Color Green { get { return new Color(0f, 1f, 0f); } }
        public static Color Red { get { return new Color(1f, 0f, 0f); } }
        public static Color Blue { get { return new Color(0f, 0f, 1f); } }
        public static Color Yellow { get { return new Color(1f, 1f, 0f); } }
        public static Color Cyan { get { return new Color(0f, 1f, 1f); } }
        public static Color White { get { return new Color(1f, 1f, 1f); } }
    }

    public class Vec3
    {
        public float X, Y, Z;
        public Vec3() { X = 0; Y = 0; Z = 0; }
        public Vec3(float x, float y, float z) { X = x; Y = y; Z = z; }
    }

    public enum LightType { Directional = 0, Point = 1, Spot = 2 }
    public enum ShapeType { Box = 0, Sphere = 1, Capsule = 2, Cylinder = 3, Plane = 4 }
    public enum CameraMode { ThirdPerson = 0, FirstPerson = 1 }

    public class Transform
    {
        public const double DegToRad = Math.PI / 180.0;

        public Vec3 Position = new Vec3();
        public Vec3 Rotation = new Vec3();
        public Vec3 Scale = new Vec3(1f, 1f, 1f);

        // Returns a column-major 4x4 matrix (matching GLM layout)
        // Order: Scale -> RotX -> RotY -> RotZ -> Translate
        public float[] ToMatrix()
        {
            float cx = (float)Math.Cos(Rotation.X * DegToRad);
            float sx = (float)Math.Sin(Rotation.X * DegToRad);
            float cy = (float)Math.Cos(Rotation.Y * DegToRad);
            float sy = (float)Math.Sin(Rotation.Y * DegToRad);
            float cz = (float)Math.Cos(Rotation.Z * DegToRad);
            float sz = (float)Math.Sin(Rotation.Z * DegToRad);

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
                r00 * Scale.X, r10 * Scale.X, r20 * Scale.X, 0f,  // column 0
                r01 * Scale.Y, r11 * Scale.Y, r21 * Scale.Y, 0f,  // column 1
                r02 * Scale.Z, r12 * Scale.Z, r22 * Scale.Z, 0f,  // column 2
                Position.X,    Position.Y,    Position.Z,    1f   // column 3
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
        public static readonly float[] Identity = new float[] {
            1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
        };

        public float[] Matrix = new float[] {
            1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
        };
    }

    public class MeshComponent
    {
        public int _MeshId = -1;
        public int _RendererEntityId = -1;
    }

    public class Movable
    {
        public float Speed = 90f;
    }

    public class Light
    {
        public LightType Type = LightType.Directional;
        public Color LightColor = Color.White;
        public float Intensity = 1f;
        public Vec3 Direction = new Vec3(0f, -1f, 0f);
        public float Radius = 10f;
        public float InnerConeDeg = 12.5f, OuterConeDeg = 17.5f;
        public int _LightIndex = -1;
    }

    public class Timer
    {
        public float Duration = 1f;
        public float Elapsed = 0f;
        public bool Repeat = false;
        public bool Finished = false;
        public string Tag = "";
    }

    public class Rigidbody
    {
        public uint _BodyId = 0;
        public bool _BodyCreated = false;
        public JPH_MotionType MotionType = JPH_MotionType.Dynamic;
        public float Friction = 0.5f;
        public float Restitution = 0.3f;
        public float LinearDamping = 0.05f;
        public float AngularDamping = 0.05f;
        public float GravityFactor = 1.0f;
    }

    public class Collider
    {
        public ShapeType Shape = ShapeType.Box;

        // Box
        public Vec3 BoxHalfExtents = new Vec3(0.5f, 0.5f, 0.5f);
        // Sphere
        public float SphereRadius = 0.5f;
        // Capsule
        public float CapsuleHalfHeight = 0.5f, CapsuleRadius = 0.3f;
        // Cylinder
        public float CylinderHalfHeight = 0.5f, CylinderRadius = 0.5f;
        // Plane
        public Vec3 PlaneNormal = new Vec3(0f, 1f, 0f);
        public float PlaneDistance = 0f;
        public float PlaneHalfExtent = 100f;

        // Debug wireframe color (default green)
        public Color DebugColor = Color.Green;
    }

    public class Camera
    {
        public Vec3 Offset = new Vec3(0f, 0f, 3f);
        public float Yaw = 0f;
        public float Pitch = 0f;
        public float Fov = 45f;
        public float LookSpeed = 90f;
        public float MouseSensitivity = 0.15f;
        public double _LastMouseX = 0.0, _LastMouseY = 0.0;
        public bool _MouseInitialized = false;
        public bool _WasEscPressed = false;

        public CameraMode Mode = CameraMode.ThirdPerson;
        public float EyeHeight = 0.8f;
        public bool _WasModeTogglePressed = false;
        public float MinDistance = 1f;
        public float MaxDistance = 20f;
        public float ZoomSpeed = 2f;
    }
}

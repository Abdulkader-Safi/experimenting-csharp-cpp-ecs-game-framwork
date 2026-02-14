namespace ECS
{
    public static class FreeCameraState
    {
        public static bool IsActive = false;

        // Position
        public static Vec3 Position = new Vec3(0f, 2f, 5f);

        // Orientation (degrees)
        public static float Yaw = 0f, Pitch = 0f;

        // Config
        public static float Fov = 45f;

        // Mouse tracking (separate from entity camera to avoid delta jumps on switch)
        public static double LastMouseX = 0.0, LastMouseY = 0.0;
        public static bool MouseInitialized = false;

        // Edge-detection state for number keys
        public static bool WasKey0Pressed = false;
        public static bool WasKey1Pressed = false;
    }
}

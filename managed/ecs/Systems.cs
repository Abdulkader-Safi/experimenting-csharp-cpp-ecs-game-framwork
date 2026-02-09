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

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_LEFT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_A))
                    tr.RotY -= mov.Speed;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_RIGHT) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_D))
                    tr.RotY += mov.Speed;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_UP) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_W))
                    tr.RotX -= mov.Speed;

                if (NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_DOWN) ||
                    NativeBridge.IsKeyPressed(NativeBridge.GLFW_KEY_S))
                    tr.RotX += mov.Speed;
            }
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
                    NativeBridge.SetEntityTransform(mc.RendererEntityId, tr.ToMatrix());
                }
            }
        }
    }
}

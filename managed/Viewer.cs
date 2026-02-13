using System;
using ECS;

class Viewer
{
    static void Main(string[] args)
    {
        if (!NativeBridge.renderer_init(800, 600, "Vulkan glTF Viewer"))
        {
            Console.WriteLine("Failed to initialize renderer");
            return;
        }

        var world = new World();
        Game.Setup(world);

#if HOT_RELOAD
        HotReload.Start();
#endif

        while (!NativeBridge.renderer_should_close())
        {
            NativeBridge.renderer_poll_events();
            world.UpdateTime();
#if HOT_RELOAD
            HotReload.TryReload(world);
#endif
            world.RunSystems();
            NativeBridge.renderer_render_frame();
        }

#if HOT_RELOAD
        HotReload.Stop();
#endif
        PhysicsWorld.Instance.Shutdown();
        NativeBridge.renderer_cleanup();
    }
}

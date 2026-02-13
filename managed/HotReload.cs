using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;

namespace ECS
{
    public static class HotReload
    {
        private static FileSystemWatcher watcher_;
        private static volatile bool reloadRequested_;
        private static System.Threading.Timer debounceTimer_;
        private static int version_;

        private static readonly string GameLogicDir =
            Path.GetFullPath("game_logic");
        private static readonly string BuildDir =
            Path.GetFullPath("build");

        public static void Start()
        {
            watcher_ = new FileSystemWatcher(GameLogicDir, "*.cs");
            watcher_.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size;
            watcher_.Changed += OnFileChanged;
            watcher_.Created += OnFileChanged;
            watcher_.EnableRaisingEvents = true;

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("[HotReload] Watching game_logic/ for changes");
            Console.ResetColor();
        }

        private static void OnFileChanged(object sender, FileSystemEventArgs e)
        {
            if (debounceTimer_ != null)
                debounceTimer_.Dispose();
            debounceTimer_ = new System.Threading.Timer(_ =>
            {
                reloadRequested_ = true;
            }, null, 300, Timeout.Infinite);
        }

        public static void TryReload(World world)
        {
            if (!reloadRequested_) return;
            reloadRequested_ = false;

            version_++;
            string dllName = "GameLogic_v" + version_ + ".dll";
            string dllPath = Path.Combine(BuildDir, dllName);
            string engineDll = Path.Combine(BuildDir, "Engine.dll");

            // Auto-discover all .cs files in game_logic/
            string[] sources = Directory.GetFiles(GameLogicDir, "*.cs");
            string sourceList = string.Join(" ", sources);

            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("[HotReload] Recompiling v" + version_ + "...");
            Console.ResetColor();

            var psi = new ProcessStartInfo
            {
                FileName = "mcs",
                Arguments = "-target:library -r:" + engineDll +
                            " -out:" + dllPath + " " + sourceList,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            string stdout, stderr;
            int exitCode;
            try
            {
                using (var proc = Process.Start(psi))
                {
                    stdout = proc.StandardOutput.ReadToEnd();
                    stderr = proc.StandardError.ReadToEnd();
                    proc.WaitForExit();
                    exitCode = proc.ExitCode;
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[HotReload] Failed to run mcs: " + ex.Message);
                Console.ResetColor();
                return;
            }

            if (exitCode != 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[HotReload] Compile FAILED:");
                if (!string.IsNullOrEmpty(stderr))
                    Console.WriteLine(stderr.TrimEnd());
                if (!string.IsNullOrEmpty(stdout))
                    Console.WriteLine(stdout.TrimEnd());
                Console.ResetColor();
                return;
            }

            // Load new assembly
            Assembly asm;
            try
            {
                asm = Assembly.LoadFrom(dllPath);
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[HotReload] Load FAILED: " + ex.Message);
                Console.ResetColor();
                return;
            }

            // Find Systems class and extract public static void(World) methods
            Type systemsType = asm.GetType("ECS.Systems");
            if (systemsType == null)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[HotReload] ECS.Systems type not found in assembly");
                Console.ResetColor();
                return;
            }

            var newMethods = new Dictionary<string, Action<World>>();
            foreach (var method in systemsType.GetMethods(
                BindingFlags.Public | BindingFlags.Static))
            {
                var parms = method.GetParameters();
                if (parms.Length == 1 && parms[0].ParameterType == typeof(World) &&
                    method.ReturnType == typeof(void))
                {
                    var del = (Action<World>)Delegate.CreateDelegate(
                        typeof(Action<World>), method);
                    newMethods[method.Name] = del;
                }
            }

            // Check if the new assembly has a Game.Setup method
            Type gameType = asm.GetType("Game");
            MethodInfo setupMethod = null;
            if (gameType != null)
            {
                setupMethod = gameType.GetMethod("Setup",
                    BindingFlags.Public | BindingFlags.Static,
                    null, new Type[] { typeof(World) }, null);
            }

            if (setupMethod != null)
            {
                // Full scene re-init: reset world and re-run Game.Setup
                world.Reset();
                setupMethod.Invoke(null, new object[] { world });

                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[HotReload] OK v" + version_ +
                                  " - Scene re-initialized");
            }
            else
            {
                // Systems-only swap (no Game class found)
                int swapped = world.ReloadSystems(newMethods);

                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("[HotReload] OK v" + version_ + " - " +
                                  swapped + " systems reloaded");
            }
            Console.ResetColor();
        }

        public static void Stop()
        {
            if (watcher_ != null)
            {
                watcher_.EnableRaisingEvents = false;
                watcher_.Dispose();
                watcher_ = null;
            }
            if (debounceTimer_ != null)
            {
                debounceTimer_.Dispose();
                debounceTimer_ = null;
            }
        }
    }
}

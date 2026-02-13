using System;
using System.Collections.Generic;

namespace ECS
{
    internal class NamedSystem
    {
        public string Name;
        public Action<World> Action;
    }

    public class World
    {
        private int nextEntityId_ = 0;
        private readonly HashSet<int> aliveEntities_ = new HashSet<int>();
        private readonly Dictionary<string, Dictionary<int, object>> components_ = new Dictionary<string, Dictionary<int, object>>();
        private readonly List<NamedSystem> systems_ = new List<NamedSystem>();

        // Time tracking (computed on native side via glfwGetTime)
        public float DeltaTime { get; private set; } = 0.016f;
        public float TotalTime { get; private set; } = 0f;

        public int Spawn()
        {
            int id = nextEntityId_++;
            aliveEntities_.Add(id);
            return id;
        }

        public bool IsAlive(int entity)
        {
            return aliveEntities_.Contains(entity);
        }

        public void Despawn(int entity)
        {
            if (!aliveEntities_.Contains(entity)) return;

            // Clean up physics body if this entity has one
            PhysicsWorld.Instance.RemoveBody(entity);

            // Clean up native renderer entity if this entity has a mesh
            var mc = GetComponent<MeshComponent>(entity);
            if (mc != null && mc.RendererEntityId >= 0)
            {
                NativeBridge.RemoveEntity(mc.RendererEntityId);
                mc.RendererEntityId = -1;
            }

            // Cascade-delete children
            var children = new List<int>();
            string hierarchyKey = typeof(Hierarchy).FullName;
            if (components_.ContainsKey(hierarchyKey))
            {
                foreach (var kvp in components_[hierarchyKey])
                {
                    var h = (Hierarchy)kvp.Value;
                    if (h.Parent == entity)
                        children.Add(kvp.Key);
                }
            }

            aliveEntities_.Remove(entity);
            foreach (var store in components_.Values)
            {
                store.Remove(entity);
            }

            // Recursively despawn children
            foreach (int child in children)
            {
                Despawn(child);
            }
        }

        public int SpawnMeshEntity(int meshId, Transform transform)
        {
            int entity = Spawn();
            AddComponent(entity, transform);
            var mc = new MeshComponent
            {
                MeshId = meshId,
                RendererEntityId = NativeBridge.CreateEntity(meshId)
            };
            AddComponent(entity, mc);
            return entity;
        }

        public void AddComponent<T>(int entity, T component) where T : class
        {
            string key = typeof(T).FullName;
            if (!components_.ContainsKey(key))
                components_[key] = new Dictionary<int, object>();
            components_[key][entity] = component;
        }

        public T GetComponent<T>(int entity) where T : class
        {
            string key = typeof(T).FullName;
            if (components_.ContainsKey(key) && components_[key].ContainsKey(entity))
                return (T)components_[key][entity];
            return null;
        }

        public bool HasComponent<T>(int entity) where T : class
        {
            string key = typeof(T).FullName;
            return components_.ContainsKey(key) && components_[key].ContainsKey(entity);
        }

        public List<int> Query(params Type[] types)
        {
            var result = new List<int>();
            foreach (int entity in aliveEntities_)
            {
                bool hasAll = true;
                foreach (Type t in types)
                {
                    string key = t.FullName;
                    if (!components_.ContainsKey(key) || !components_[key].ContainsKey(entity))
                    {
                        hasAll = false;
                        break;
                    }
                }
                if (hasAll) result.Add(entity);
            }
            return result;
        }

        public void AddSystem(Action<World> system)
        {
            systems_.Add(new NamedSystem { Name = system.Method.Name, Action = system });
        }

        public void UpdateTime()
        {
            NativeBridge.renderer_update_time();
            DeltaTime = NativeBridge.renderer_get_delta_time();
            TotalTime = NativeBridge.renderer_get_total_time();
        }

        public void RunSystems()
        {
            foreach (var sys in systems_)
            {
                sys.Action(this);
            }
        }

        public void Reset()
        {
            // Remove all physics bodies before despawning entities
            PhysicsWorld.Instance.RemoveAllBodies();

            // Despawn all entities (cleans up native renderer entities via MeshComponent)
            var ids = new List<int>(aliveEntities_);
            foreach (int id in ids)
            {
                Despawn(id);
            }

            // Clear all component stores and systems
            components_.Clear();
            systems_.Clear();

            // Clear all debug wireframe entities on the native side
            NativeBridge.ClearDebugEntities();

            // Clear all 8 light slots on the native side
            for (int i = 0; i < 8; i++)
                NativeBridge.ClearLight(i);

            nextEntityId_ = 0;
        }

        public int ReloadSystems(Dictionary<string, Action<World>> newMethods)
        {
            int swapped = 0;
            for (int i = 0; i < systems_.Count; i++)
            {
                Action<World> replacement;
                if (newMethods.TryGetValue(systems_[i].Name, out replacement))
                {
                    systems_[i].Action = replacement;
                    swapped++;
                }
            }
            return swapped;
        }
    }
}

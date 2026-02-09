using System;
using System.Collections.Generic;

namespace ECS
{
    public class World
    {
        private int nextEntityId_ = 0;
        private readonly HashSet<int> aliveEntities_ = new HashSet<int>();
        private readonly Dictionary<string, Dictionary<int, object>> components_ = new Dictionary<string, Dictionary<int, object>>();
        private readonly List<Action<World>> systems_ = new List<Action<World>>();

        // Time tracking (computed on native side via glfwGetTime)
        public float DeltaTime { get; private set; } = 0.016f;
        public float TotalTime { get; private set; } = 0f;

        public int Spawn()
        {
            int id = nextEntityId_++;
            aliveEntities_.Add(id);
            return id;
        }

        public void Despawn(int entity)
        {
            aliveEntities_.Remove(entity);
            foreach (var store in components_.Values)
            {
                store.Remove(entity);
            }
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
            systems_.Add(system);
        }

        public void UpdateTime()
        {
            NativeBridge.renderer_update_time();
            DeltaTime = NativeBridge.renderer_get_delta_time();
            TotalTime = NativeBridge.renderer_get_total_time();
        }

        public void RunSystems()
        {
            foreach (var system in systems_)
            {
                system(this);
            }
        }
    }
}

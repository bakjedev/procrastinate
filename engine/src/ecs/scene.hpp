#pragma once
#include <cstdint>
#include <entt/entt.hpp>

class Scene
{
public:
  uint32_t Create();
  void Destroy(uint32_t entity);

  template<typename C, typename... Args>
  C *AddComponent(uint32_t entity, Args &&...args);

  entt::registry &registry() { return registry_; }

private:
  entt::registry registry_;
};

template<typename C, typename... Args>
C *Scene::AddComponent(uint32_t entity, Args &&...args)
{
  auto entt_entity = static_cast<entt::entity>(entity);
  return &registry_.emplace_or_replace<C>(entt_entity, std::forward<Args>(args)...);
}

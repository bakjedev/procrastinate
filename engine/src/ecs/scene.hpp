#pragma once
#include <cstdint>
#include <entt/entt.hpp>

class Scene {
 public:
  uint32_t create();
  void destroy(uint32_t entity);

  template <typename C, typename... Args>
  bool addComponent(uint32_t entity, Args &&...args);

  entt::registry &registry() { return m_registry; }

 private:
  entt::registry m_registry;
};

template <typename C, typename... Args>
bool Scene::addComponent(uint32_t entity, Args &&...args) {
  auto entt_entity = static_cast<entt::entity>(entity);
  if (m_registry.any_of<C>(entt_entity)) {
    return false;
  }
  m_registry.emplace_or_replace<C>(entt_entity, std::forward<Args>(args)...);
  return true;
}
#pragma once
#include <cstdint>
#include <entt/entt.hpp>

class Scene {
public:
  uint32_t create();
  void destroy(uint32_t entity);

  template <typename C, typename... Args>
  C *addComponent(uint32_t entity, Args &&...args);

  entt::registry &registry() { return m_registry; }

private:
  entt::registry m_registry;
};

template <typename C, typename... Args>
C *Scene::addComponent(uint32_t entity, Args &&...args) {
  auto entt_entity = static_cast<entt::entity>(entity);
  return &m_registry.emplace_or_replace<C>(entt_entity,
                                           std::forward<Args>(args)...);
}
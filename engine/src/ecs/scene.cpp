#include "scene.hpp"

uint32_t Scene::create() { return static_cast<uint32_t>(m_registry.create()); }

void Scene::destroy(uint32_t entity) {
  m_registry.destroy(static_cast<entt::entity>(entity));
}

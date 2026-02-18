#include "scene.hpp"

uint32_t Scene::Create() { return static_cast<uint32_t>(registry_.create()); }

void Scene::Destroy(uint32_t entity) { registry_.destroy(static_cast<entt::entity>(entity)); }

#pragma once

#include <chrono>

#include "ecs/components/camera_component.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/scene.hpp"
#include "events.hpp"
#include "input/input.hpp"
#include "render/vk_renderer.hpp"
#include "tracy/Tracy.hpp"
#include "window.hpp"

template<Application App>
void Engine::run(App& app)
{
  app.init(*this);

  using clock = std::chrono::steady_clock;

  auto lastTime = clock::now();
  float accumulator = 0.0F;
  constexpr float fixedDt = 1.0F / 60.0F;

  while (!m_window->shouldQuit())
  {
    ZoneScopedN("EngineLoop");

    auto currentTime = clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    m_eventManager->poll();
    m_window->update();
    m_input->update();
    m_renderer->clearLines();

    app.update(deltaTime);

    accumulator += deltaTime;
    while (accumulator >= fixedDt)
    {
      app.fixedUpdate(fixedDt);
      accumulator -= fixedDt;
    }

    m_renderer->clearMeshes();
    auto view = m_scene->registry().view<CMesh, CTransform>();
    for (const auto entity: view)
    {
      const auto& mesh = view.get<CMesh>(entity);
      const auto& transform = view.get<CTransform>(entity);
      m_renderer->renderMesh(transform.world, mesh.mesh->renderer_id);
    }

    const auto cameraView = m_scene->registry().view<CCamera, CTransform>();
    entt::entity cameraEntity = entt::null;
    auto it = cameraView.begin();
    if (it != cameraView.end())
    {
      cameraEntity = *it;
    }
    if (cameraEntity != entt::null)
    {
      const auto& camera = cameraView.get<CCamera>(cameraEntity);
      const auto& transform = cameraView.get<CTransform>(cameraEntity);
      app.render();
      m_renderer->run(transform.world, camera.fov);
    }
    FrameMark;
  }

  app.shutdown();
}

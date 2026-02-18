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

  while (!window_->ShouldQuit())
  {
    ZoneScopedN("EngineLoop");

    auto currentTime = clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    event_manager_->poll();
    window_->update();
    input_->update();
    renderer_->ClearLines();

    app.update(deltaTime);

    accumulator += deltaTime;
    while (accumulator >= fixedDt)
    {
      app.fixedUpdate(fixedDt);
      accumulator -= fixedDt;
    }

    renderer_->ClearMeshes();
    auto view = scene_->registry().view<CMesh, CTransform>();
    for (const auto entity: view)
    {
      const auto& mesh = view.get<CMesh>(entity);
      const auto& transform = view.get<CTransform>(entity);
      renderer_->RenderMesh(transform.world, mesh.mesh->renderer_id);
    }

    const auto cameraView = scene_->registry().view<CCamera, CTransform>();
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
      renderer_->run(transform.world, camera.fov);
    }
    FrameMark;
  }

  app.shutdown();
}

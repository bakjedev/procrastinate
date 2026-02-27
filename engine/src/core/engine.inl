#pragma once

#include <chrono>

#include "ecs/components/camera_component.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/scene.hpp"
#include "events.hpp"
#include "input/input.hpp"
#include "render/vk_renderer.hpp"
#include "resource/resource_manager.hpp"
#include "tracy/Tracy.hpp"
#include "window.hpp"

template<Application App>
void Engine::Run(App& app)
{
  app.Init(*this);
  resource_manager_->GetStorage<MeshResource>().AddOnDestroyCallback(
      ResourceCallback<MeshResource>::Create<&VulkanRenderer::OnMeshResourceDestroyed>(renderer_.get()));

  using Clock = std::chrono::steady_clock;

  auto last_time = Clock::now();
  float accumulator = 0.0F;
  constexpr float fixed_dt = 1.0F / 60.0F;

  while (!window_->ShouldQuit())
  {
    ZoneScopedN("EngineLoop");

    const auto current_time = Clock::now();
    const float delta_time = std::chrono::duration<float>(current_time - last_time).count();
    last_time = current_time;

    event_manager_->poll();
    window_->update();
    input_->update();
    renderer_->ClearLines();

    app.Update(delta_time);

    accumulator += delta_time;
    while (accumulator >= fixed_dt)
    {
      app.FixedUpdate(fixed_dt);
      accumulator -= fixed_dt;
    }

    renderer_->ClearMeshes();
    auto view = scene_->registry().view<CMesh, CTransform>();
    for (const auto entity: view)
    {
      const auto& mesh = view.get<CMesh>(entity);
      const auto& transform = view.get<CTransform>(entity);
      renderer_->RenderMesh(transform.world, mesh.mesh_id, mesh.texture_id);
    }

    const auto camera_view = scene_->registry().view<CCamera, CTransform>();
    entt::entity camera_entity = entt::null;
    auto it = camera_view.begin();
    if (it != camera_view.end())
    {
      camera_entity = *it;
    }
    if (camera_entity != entt::null)
    {
      const auto& camera = camera_view.get<CCamera>(camera_entity);
      const auto& transform = camera_view.get<CTransform>(camera_entity);
      app.Render();
      renderer_->run(transform.world, camera.fov);
    }
    FrameMark;
  }

  app.Shutdown();
}

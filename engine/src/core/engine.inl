#pragma once

#include "ecs/components/mesh_component.hpp"
#ifndef ENGINE_HPP_INCLUDED
#include "engine.hpp"  // for lsp
#endif

#include <chrono>

#include "events.hpp"
#include "input/input.hpp"
#include "render/vk_renderer.hpp"
#include "window.hpp"
#include "ecs/scene.hpp"

template <Application App>
void Engine::run(App& app) {
  app.init(*this);

  using clock = std::chrono::steady_clock;

  auto lastTime = clock::now();
  float accumulator = 0.0F;
  constexpr float fixedDt = 1.0F / 60.0F;

  while (!m_window->shouldQuit()) {
    auto currentTime = clock::now();
    float deltaTime =
        std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    m_eventManager->poll();
    m_window->update();
    m_input->update();

    app.update(deltaTime);

    accumulator += deltaTime;
    while (accumulator >= fixedDt) {
      app.fixedUpdate(fixedDt);
      accumulator -= fixedDt;
    }

    m_renderer->clear();
    auto view = m_scene->registry().view<MeshComponent>();
    for (const auto entity : view) {
      const auto& mesh = view.get<MeshComponent>(entity);
      m_renderer->renderMesh(mesh.mesh->renderer_id);
    }

    app.render();
    m_renderer->run();
  }

  app.shutdown();
}

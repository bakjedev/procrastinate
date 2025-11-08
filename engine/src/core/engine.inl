#pragma once

#ifndef ENGINE_HPP_INCLUDED
#include "engine.hpp"  // for lsp
#endif

#include <chrono>

#include "events.hpp"
#include "input/input.hpp"
#include "window.hpp"

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

    app.render();
  }

  app.shutdown();
}
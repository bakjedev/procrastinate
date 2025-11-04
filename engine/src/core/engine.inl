#pragma once

#ifndef ENGINE_HPP_INCLUDED
#include "engine.hpp"  // for lsp
#endif

#include "events.hpp"
#include "input/input.hpp"
#include "window.hpp"

template <Application App>
void Engine::run(App& app) {
  app.init();

  while (!m_window->shouldQuit()) {
    m_eventManager->poll();
    m_window->update(*m_eventManager);
    m_input->update(*m_eventManager);

    app.update(1.0F);
    app.fixedUpdate(1.0F);

    app.render();
  }

  app.shutdown();
}
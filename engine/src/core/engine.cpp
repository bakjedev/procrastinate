#include "engine.hpp"

#include <cassert>
#include <memory>

#include "events.hpp"
#include "util/util.hpp"
#include "window.hpp"

Engine::Engine() {
  Util::print("HELLO \n");
  m_eventManager = std::make_unique<EventManager>();
  m_window = std::make_unique<Window>(1920, 1080, false);
}

Engine::~Engine() { Util::print("BYE \n"); }

EventManager& Engine::getEventManager() const {
  assert(m_eventManager && "No event manager!!");
  return *m_eventManager;
}

Window& Engine::getWindow() const {
  assert(m_window && "No window!");
  return *m_window;
}
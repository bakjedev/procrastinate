#include "engine.hpp"

#include <cassert>
#include <memory>

#include "events.hpp"
#include "input/input.hpp"
#include "util/util.hpp"
#include "window.hpp"

Engine::Engine() {
  Util::print("HELLO \n");
  m_eventManager = std::make_unique<EventManager>();
  m_window = std::make_unique<Window>(
      WindowInfo{.width = 1920, .height = 1080, .fullscreen = false});
  m_input = std::make_unique<Input>();
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

Input& Engine::getInput() const {
  assert(m_input && "No input!");
  return *m_input;
}
#include "engine.hpp"

#include <cassert>
#include <memory>

#include "events.hpp"
#include "input/input.hpp"
#include "render/vk_renderer.hpp"
#include "util/util.hpp"
#include "window.hpp"

Engine::Engine() {
  m_eventManager = std::make_unique<EventManager>();
  m_window = std::make_unique<Window>(WindowInfo{
      .width = 1920, .height = 1080, .fullscreen = false, .title = "meowl"});
  m_input = std::make_unique<Input>();
  m_renderer = std::make_unique<VulkanRenderer>(m_window->get());
  Util::println("Engine initialized");
}

Engine::~Engine() { Util::println("Engine destroyed"); }

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

VulkanRenderer& Engine::getRenderer() const {
  assert(m_renderer && "No renderer!");
  return *m_renderer;
}
#include "engine.hpp"

#include <cassert>
#include <memory>

#include "core/imgui.hpp"
#include "ecs/scene.hpp"
#include "events.hpp"
#include "input/input.hpp"
#include "render/vk_renderer.hpp"
#include "resource/resource_manager.hpp"
#include "util/print.hpp"
#include "window.hpp"

Engine::Engine()
{
  Util::println("procrastinating");

  constexpr uint32_t width = 1920;
  constexpr uint32_t height = 1080;

  m_eventManager = std::make_unique<EventManager>();
  m_window = std::make_unique<Window>(
      WindowInfo{.width = width, .height = height, .fullscreen = false, .title = "meowl"}, *m_eventManager);
  m_input = std::make_unique<Input>(*m_eventManager);
  m_resourceManager = std::make_unique<ResourceManager>();
  ImGuiSystem::initialize(m_window.get());
  m_renderer = std::make_unique<VulkanRenderer>(m_window.get(), *m_resourceManager, *m_eventManager);
  m_scene = std::make_unique<Scene>();

  Util::println("Engine initialized");
}

Engine::~Engine() { Util::println("Engine destroyed"); }

EventManager& Engine::getEventManager() const
{
  assert(m_eventManager && "No event manager!!");
  return *m_eventManager;
}

Window& Engine::getWindow() const
{
  assert(m_window && "No window!");
  return *m_window;
}

Input& Engine::getInput() const
{
  assert(m_input && "No input!");
  return *m_input;
}

ResourceManager& Engine::getResourceManager() const
{
  assert(m_resourceManager && "No resource manager!");
  return *m_resourceManager;
}

VulkanRenderer& Engine::getRenderer() const
{
  assert(m_renderer && "No renderer!");
  return *m_renderer;
}

Scene& Engine::getScene() const
{
  assert(m_scene && "No scene!");
  return *m_scene;
}

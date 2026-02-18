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
  util::println("procrastinating");

  constexpr uint32_t width = 1920;
  constexpr uint32_t height = 1080;

  event_manager_ = std::make_unique<EventManager>();
  window_ = std::make_unique<Window>(
      WindowInfo{.width = width, .height = height, .fullscreen = false, .title = "meowl"}, *event_manager_);
  input_ = std::make_unique<Input>(*event_manager_);
  resource_manager_ = std::make_unique<ResourceManager>();
  im_gui_system::Initialize(window_.get());
  renderer_ = std::make_unique<VulkanRenderer>(window_.get(), *resource_manager_, *event_manager_);
  scene_ = std::make_unique<Scene>();

  util::println("Engine initialized");
}

Engine::~Engine() { util::println("Engine destroyed"); }

EventManager& Engine::GetEventManager() const
{
  assert(event_manager_ && "No event manager!!");
  return *event_manager_;
}

Window& Engine::GetWindow() const
{
  assert(window_ && "No window!");
  return *window_;
}

Input& Engine::GetInput() const
{
  assert(input_ && "No input!");
  return *input_;
}

ResourceManager& Engine::GetResourceManager() const
{
  assert(resource_manager_ && "No resource manager!");
  return *resource_manager_;
}

VulkanRenderer& Engine::GetRenderer() const
{
  assert(renderer_ && "No renderer!");
  return *renderer_;
}

Scene& Engine::GetScene() const
{
  assert(scene_ && "No scene!");
  return *scene_;
}

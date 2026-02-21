#pragma once

#include <memory>

class EventManager;
class Window;
class Input;
class ResourceManager;
class VulkanRenderer;
class Engine;
class Scene;

template<typename T>
concept Application = requires(T app, float delta_time, Engine& engine) {
  { app.Init(engine) } -> std::same_as<void>;
  { app.Update(delta_time) } -> std::same_as<void>;
  { app.FixedUpdate(delta_time) } -> std::same_as<void>;
  { app.Render() } -> std::same_as<void>;
  { app.Shutdown() } -> std::same_as<void>;
};

class Engine
{
public:
  Engine();
  Engine(const Engine&) = delete;
  Engine(Engine&&) = default;
  Engine& operator=(const Engine&) = delete;
  Engine& operator=(Engine&&) = default;
  ~Engine();

  template<Application App>
  void Run(App& app);

  [[nodiscard]] EventManager& GetEventManager() const;
  [[nodiscard]] Window& GetWindow() const;
  [[nodiscard]] Input& GetInput() const;
  [[nodiscard]] ResourceManager& GetResourceManager() const;
  [[nodiscard]] VulkanRenderer& GetRenderer() const;
  [[nodiscard]] Scene& GetScene() const;

private:
  std::unique_ptr<EventManager> event_manager_;
  std::unique_ptr<Window> window_;
  std::unique_ptr<Input> input_;
  std::unique_ptr<ResourceManager> resource_manager_;
  std::unique_ptr<VulkanRenderer> renderer_;
  std::unique_ptr<Scene> scene_;
};

#include "engine.inl" // IWYU pragma: keep

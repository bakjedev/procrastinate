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
concept Application = requires(T app, float deltaTime, Engine& engine) {
  { app.init(engine) } -> std::same_as<void>;
  { app.update(deltaTime) } -> std::same_as<void>;
  { app.fixedUpdate(deltaTime) } -> std::same_as<void>;
  { app.render() } -> std::same_as<void>;
  { app.shutdown() } -> std::same_as<void>;
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
  void run(App& app);

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

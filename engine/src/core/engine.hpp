#pragma once

#define ENGINE_HPP_INCLUDED  // for LSP

#include <memory>

class EventManager;
class Window;
class Input;
class VulkanRenderer;

template <typename T>
concept Application = requires(T app, float deltaTime) {
  { app.init() } -> std::same_as<void>;
  { app.update(deltaTime) } -> std::same_as<void>;
  { app.fixedUpdate(deltaTime) } -> std::same_as<void>;
  { app.render() } -> std::same_as<void>;
  { app.shutdown() } -> std::same_as<void>;
};

class Engine {
 public:
  Engine();
  Engine(const Engine&) = delete;
  Engine(Engine&&) = default;
  Engine& operator=(const Engine&) = delete;
  Engine& operator=(Engine&&) = default;
  ~Engine();

  template <Application App>
  void run(App& app);

  [[nodiscard]] EventManager& getEventManager() const;
  [[nodiscard]] Window& getWindow() const;
  [[nodiscard]] Input& getInput() const;
  [[nodiscard]] VulkanRenderer& getRenderer() const;

 private:
  std::unique_ptr<EventManager> m_eventManager;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<Input> m_input;
  std::unique_ptr<VulkanRenderer> m_renderer;
};

#include "engine.inl"  // IWYU pragma: keep

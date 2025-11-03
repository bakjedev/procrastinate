#pragma once

#include <memory>

class EventManager;
class Window;
class Input;
class VulkanRenderer;

class Engine {
 public:
  Engine();
  ~Engine();

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
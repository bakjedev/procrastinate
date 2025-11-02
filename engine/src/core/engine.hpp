#pragma once

#include <memory>

class EventManager;
class Window;
class Input;

class Engine {
 public:
  Engine();
  ~Engine();

  [[nodiscard]] EventManager& getEventManager() const;
  [[nodiscard]] Window& getWindow() const;
  [[nodiscard]] Input& getInput() const;

 private:
  std::unique_ptr<EventManager> m_eventManager;
  std::unique_ptr<Window> m_window;
  std::unique_ptr<Input> m_input;
};
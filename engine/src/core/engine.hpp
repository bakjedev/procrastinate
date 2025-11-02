#pragma once

#include <memory>

class EventManager;
class Window;

class Engine {
 public:
  Engine();
  ~Engine();

  [[nodiscard]] EventManager& getEventManager() const;
  [[nodiscard]] Window& getWindow() const;

 private:
  std::unique_ptr<EventManager> m_eventManager;
  std::unique_ptr<Window> m_window;
};
#pragma once
#include <SDL3/SDL.h>

#include <utility>

class EventManager;

struct WindowInfo {
  uint32_t width;
  uint32_t height;
  bool fullscreen;
  const char* title;
};

class Window {
 public:
  Window(const WindowInfo& info, EventManager& eventManager);
  Window(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(const Window&) = delete;
  Window& operator=(Window&&) = delete;
  ~Window();

  [[nodiscard]] SDL_Window* get() const;
  [[nodiscard]] bool shouldQuit() const;

  void quit();

  void update();

  [[nodiscard]] std::pair<uint32_t, uint32_t> getWindowSize() const;

 private:
  bool m_quit = false;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  bool m_fullscreen = false;

  EventManager* m_eventManager;

  SDL_Window* m_window = nullptr;
};

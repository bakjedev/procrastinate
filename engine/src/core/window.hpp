#pragma once
#include <SDL3/SDL.h>

class EventManager;

struct WindowInfo {
  uint32_t width;
  uint32_t height;
  bool fullscreen;
  const char* title;
};

class Window {
 public:
  Window(const WindowInfo& info);
  ~Window();

  [[nodiscard]] SDL_Window* get() const;
  [[nodiscard]] bool shouldQuit() const;

  void update(const EventManager& event_manager);

 private:
  bool m_quit = false;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  bool m_fullscreen = false;

  SDL_Window* m_window = nullptr;
};

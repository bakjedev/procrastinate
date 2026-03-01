#pragma once
#include <SDL3/SDL.h>

#include <utility>

class EventManager;

struct WindowInfo
{
  uint32_t width;
  uint32_t height;
  bool fullscreen;
  const char* title;
};

class Window
{
public:
  Window(const WindowInfo& info, EventManager& event_manager);
  Window(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(const Window&) = delete;
  Window& operator=(Window&&) = delete;
  ~Window();

  [[nodiscard]] SDL_Window* get() const;
  [[nodiscard]] bool ShouldQuit() const;

  void quit();

  void update();

  [[nodiscard]] std::pair<uint32_t, uint32_t> GetWindowSize() const;
  static std::pair<uint32_t, uint32_t> GetDisplaySize();

private:
  bool quit_ = false;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  bool fullscreen_ = false;

  EventManager* event_manager_;

  SDL_Window* window_ = nullptr;
};

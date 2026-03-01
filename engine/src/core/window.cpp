#include "window.hpp"

#include <stdexcept>

#include "core/events.hpp"
#include "events.hpp"

Window::Window(const WindowInfo& info, EventManager& event_manager) :
    width_(info.width), height_(info.height), fullscreen_(info.fullscreen), event_manager_(&event_manager)
{
  if (width_ == 0 || height_ == 0)
  {
    throw std::runtime_error("Invalid window size");
  }

  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
  if (fullscreen_)
  {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  window_ = SDL_CreateWindow(info.title, static_cast<int>(width_), static_cast<int>(height_), flags);
  if (window_ == nullptr)
  {
    throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
  }
}

Window::~Window()
{
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

SDL_Window* Window::get() const { return window_; }

bool Window::ShouldQuit() const { return quit_; }

void Window::quit() { quit_ = true; }

void Window::update()
{
  for (const auto& event: event_manager_->GetEvents())
  {
    if (event.type == EventType::kQuit)
    {
      quit_ = true;
    }
  }
}

std::pair<uint32_t, uint32_t> Window::GetWindowSize() const
{
  int width{};
  int height{};
  SDL_GetWindowSizeInPixels(window_, &width, &height);

  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}
std::pair<uint32_t, uint32_t> Window::GetDisplaySize()
{
  const SDL_DisplayID display = SDL_GetPrimaryDisplay();
  SDL_Rect rect;
  if (!SDL_GetDisplayBounds(display, &rect))
  {
    throw std::runtime_error(std::string("Failed to get display bounds: ") + SDL_GetError());
  }
  return {static_cast<uint32_t>(rect.w), static_cast<uint32_t>(rect.h)};
}

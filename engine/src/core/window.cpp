#include "window.hpp"

#include <stdexcept>

#include "events.hpp"

Window::Window(const WindowInfo& info)
    : m_width(info.width),
      m_height(info.height),
      m_fullscreen(info.fullscreen) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error(std::string("Failed to initialize SDL: ") +
                             SDL_GetError());
  }

  if (m_width == 0 || m_height == 0) {
    throw std::runtime_error("Invalid window size");
  }

  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
  if (m_fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  m_window = SDL_CreateWindow(info.title, static_cast<int>(m_width),
                              static_cast<int>(m_height), flags);
  if (m_window == nullptr) {
    throw std::runtime_error(std::string("Failed to create window: ") +
                             SDL_GetError());
  }
}

Window::~Window() {
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

SDL_Window* Window::get() const { return m_window; }

bool Window::shouldQuit() const { return m_quit; }

void Window::update(const EventManager& event_manager) {
  for (const auto& event : event_manager.getEvents()) {
    if (event.type == EventType::Quit) {
      m_quit = true;
    }
  }
}

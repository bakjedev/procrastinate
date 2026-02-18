#include "window.hpp"

#include <stdexcept>

#include "core/events.hpp"
#include "events.hpp"

Window::Window(const WindowInfo& info, EventManager& eventManager) :
    m_width(info.width), m_height(info.height), m_fullscreen(info.fullscreen), m_eventManager(&eventManager)
{
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
  }

  if (m_width == 0 || m_height == 0)
  {
    throw std::runtime_error("Invalid window size");
  }

  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
  if (m_fullscreen)
  {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  m_window = SDL_CreateWindow(info.title, static_cast<int>(m_width), static_cast<int>(m_height), flags);
  if (m_window == nullptr)
  {
    throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
  }
}

Window::~Window()
{
  SDL_DestroyWindow(m_window);
  SDL_Quit();
}

SDL_Window* Window::get() const { return m_window; }

bool Window::shouldQuit() const { return m_quit; }

void Window::quit() { m_quit = true; }

void Window::update()
{
  for (const auto& event: m_eventManager->getEvents())
  {
    if (event.type == EventType::Quit)
    {
      m_quit = true;
    }
  }
}

std::pair<uint32_t, uint32_t> Window::getWindowSize() const
{
  int width{};
  int height{};
  SDL_GetWindowSizeInPixels(m_window, &width, &height);

  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

#include "events.hpp"

#include <optional>

#include "SDL3/SDL_events.h"
#include "core/imgui.hpp"

void EventManager::poll()
{
  clear();
  m_events.reserve(expectedEvents);

  SDL_Event sdlEvent;
  while (SDL_PollEvent(&sdlEvent))
  {
    std::optional<Event> event{};

    switch (sdlEvent.type)
    {
      case SDL_EVENT_QUIT:
        event = Event{.type = EventType::Quit, .data = std::monostate{}};
        break;
      case SDL_EVENT_KEY_DOWN:
        event = Event{.type = EventType::KeyDown,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdlEvent.key.scancode)}};
        break;
      case SDL_EVENT_KEY_UP:
        event = Event{.type = EventType::KeyUp,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdlEvent.key.scancode)}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        event = Event{.type = EventType::MouseButtonDown, .data = InputData{.scancode = sdlEvent.button.button}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        event = Event{.type = EventType::MouseButtonUp, .data = InputData{.scancode = sdlEvent.button.button}};
        break;
      case SDL_EVENT_MOUSE_MOTION:
        event = Event{.type = EventType::MouseMotion,
                      .data = MotionData{.x = sdlEvent.motion.x,
                                         .y = sdlEvent.motion.y,
                                         .dx = sdlEvent.motion.xrel,
                                         .dy = sdlEvent.motion.yrel}};
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        event = Event{.type = EventType::MouseWheel, .data = WheelData{.scroll = sdlEvent.wheel.y}};
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        event = Event{.type = EventType::WindowResized,
                      .data = WindowResizeData{.width = static_cast<uint32_t>(sdlEvent.window.data1),
                                               .height = static_cast<uint32_t>(sdlEvent.window.data2)}};
        break;
      default:
        break;
    }

    if (event)
    {
      m_events.push_back(*event);
    }
    ImGuiSystem::processEvent(&sdlEvent);
  }
}

const std::vector<Event>& EventManager::getEvents() const { return m_events; }

void EventManager::clear() { m_events.clear(); }

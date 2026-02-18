#include "events.hpp"

#include <optional>

#include "SDL3/SDL_events.h"
#include "core/imgui.hpp"

void EventManager::poll()
{
  clear();
  events_.reserve(expected_events_);

  SDL_Event sdlEvent;
  while (SDL_PollEvent(&sdlEvent))
  {
    std::optional<Event> event{};

    switch (sdlEvent.type)
    {
      case SDL_EVENT_QUIT:
        event = Event{.type = EventType::kQuit, .data = std::monostate{}};
        break;
      case SDL_EVENT_KEY_DOWN:
        event = Event{.type = EventType::kKeyDown,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdlEvent.key.scancode)}};
        break;
      case SDL_EVENT_KEY_UP:
        event = Event{.type = EventType::kKeyUp,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdlEvent.key.scancode)}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        event = Event{.type = EventType::kMouseButtonDown, .data = InputData{.scancode = sdlEvent.button.button}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        event = Event{.type = EventType::kMouseButtonUp, .data = InputData{.scancode = sdlEvent.button.button}};
        break;
      case SDL_EVENT_MOUSE_MOTION:
        event = Event{.type = EventType::kMouseMotion,
                      .data = MotionData{.x = sdlEvent.motion.x,
                                         .y = sdlEvent.motion.y,
                                         .dx = sdlEvent.motion.xrel,
                                         .dy = sdlEvent.motion.yrel}};
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        event = Event{.type = EventType::kMouseWheel, .data = WheelData{.scroll = sdlEvent.wheel.y}};
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        event = Event{.type = EventType::kWindowResized,
                      .data = WindowResizeData{.width = static_cast<uint32_t>(sdlEvent.window.data1),
                                               .height = static_cast<uint32_t>(sdlEvent.window.data2)}};
        break;
      default:
        break;
    }

    if (event)
    {
      events_.push_back(*event);
    }
    im_gui_system::ProcessEvent(&sdlEvent);
  }
}

const std::vector<Event>& EventManager::getEvents() const { return events_; }

void EventManager::clear() { events_.clear(); }

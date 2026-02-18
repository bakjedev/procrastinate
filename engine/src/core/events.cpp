#include "events.hpp"

#include <optional>

#include "SDL3/SDL_events.h"
#include "core/imgui.hpp"

void EventManager::poll()
{
  clear();
  events_.reserve(expected_events_);

  SDL_Event sdl_event;
  while (SDL_PollEvent(&sdl_event))
  {
    std::optional<Event> event{};

    switch (sdl_event.type)
    {
      case SDL_EVENT_QUIT:
        event = Event{.type = EventType::kQuit, .data = std::monostate{}};
        break;
      case SDL_EVENT_KEY_DOWN:
        event = Event{.type = EventType::kKeyDown,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdl_event.key.scancode)}};
        break;
      case SDL_EVENT_KEY_UP:
        event = Event{.type = EventType::kKeyUp,
                      .data = InputData{.scancode = static_cast<uint32_t>(sdl_event.key.scancode)}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        event = Event{.type = EventType::kMouseButtonDown, .data = InputData{.scancode = sdl_event.button.button}};
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        event = Event{.type = EventType::kMouseButtonUp, .data = InputData{.scancode = sdl_event.button.button}};
        break;
      case SDL_EVENT_MOUSE_MOTION:
        event = Event{.type = EventType::kMouseMotion,
                      .data = MotionData{.x = sdl_event.motion.x,
                                         .y = sdl_event.motion.y,
                                         .dx = sdl_event.motion.xrel,
                                         .dy = sdl_event.motion.yrel}};
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        event = Event{.type = EventType::kMouseWheel, .data = WheelData{.scroll = sdl_event.wheel.y}};
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        event = Event{.type = EventType::kWindowResized,
                      .data = WindowResizeData{.width = static_cast<uint32_t>(sdl_event.window.data1),
                                               .height = static_cast<uint32_t>(sdl_event.window.data2)}};
        break;
      default:
        break;
    }

    if (event)
    {
      events_.push_back(*event);
    }
    im_gui_system::ProcessEvent(&sdl_event);
  }
}

const std::vector<Event>& EventManager::GetEvents() const { return events_; }

void EventManager::clear() { events_.clear(); }

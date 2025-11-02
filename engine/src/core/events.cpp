#include "events.hpp"

#include "SDL3/SDL_events.h"

void EventManager::poll() {
  clear();
  m_events.reserve(64);

  SDL_Event sdlEvent;
  while (SDL_PollEvent(&sdlEvent)) {
    Event event;
    bool valid = true;

    switch (sdlEvent.type) {
      case SDL_EVENT_QUIT:
        event.type = EventType::Quit;
        break;
      case SDL_EVENT_KEY_DOWN:
        event.type = EventType::KeyDown;
        event.data.input.scancode = sdlEvent.key.key;
        break;
      case SDL_EVENT_KEY_UP:
        event.type = EventType::KeyUp;
        event.data.input.scancode = sdlEvent.key.key;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        event.type = EventType::MouseButtonDown;
        event.data.input.scancode = sdlEvent.button.button;
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        event.type = EventType::MouseButtonUp;
        event.data.input.scancode = sdlEvent.button.button;
        break;
      default:
        valid = false;
        break;
    }

    if (valid) {
      m_events.push_back(event);
    }
  }
}

const std::vector<Event>& EventManager::getEvents() const { return m_events; }

void EventManager::clear() { m_events.clear(); }

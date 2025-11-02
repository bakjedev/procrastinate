#include "events.hpp"
// #include <SDL3/SDL.h>

void EventManager::poll() {
  clear();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    m_events.push_back(event);
  }
}

const std::vector<SDL_Event>& EventManager::getEvents() const {
  return m_events;
}

void EventManager::clear() { m_events.clear(); }

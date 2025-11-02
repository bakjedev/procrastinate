#pragma once
#include <SDL3/SDL.h>

#include <vector>

class EventManager {
 public:
  void poll();

  [[nodiscard]] const std::vector<SDL_Event>& getEvents() const;

 private:
  std::vector<SDL_Event> m_events;

  void clear();
};

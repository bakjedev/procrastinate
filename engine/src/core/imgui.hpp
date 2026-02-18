#pragma once

class Window;
class EventManager;
union SDL_Event;

namespace ImGuiSystem
{
  void initialize(Window* window);

  void processEvent(const SDL_Event* event);
} // namespace ImGuiSystem

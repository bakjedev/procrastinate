#pragma once

class Window;
class EventManager;
union SDL_Event;

namespace im_gui_system
{
  void Initialize(const Window* window);

  void ProcessEvent(const SDL_Event* event);
} // namespace im_gui_system

#include <SDL3/SDL_video.h>

#include <cstdint>

#include "events.hpp"

struct Width {
  uint32_t value;
};

struct Height {
  uint32_t value;
};

class Window {
 public:
  Window(uint32_t width, uint32_t height, bool fullscreen = false);
  ~Window();

  [[nodiscard]] SDL_Window* get() const;
  [[nodiscard]] bool shouldQuit() const;

  void update(const EventManager& event_manager);

 private:
  bool m_quit = false;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  bool m_fullscreen = false;

  SDL_Window* m_window = nullptr;
};

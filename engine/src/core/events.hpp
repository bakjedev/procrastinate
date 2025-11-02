#pragma once
#include <cstdint>
#include <vector>


enum class EventType : uint8_t {
  Quit,
  KeyDown,
  KeyUp,
  MouseButtonDown,
  MouseButtonUp,
};

struct Event {
  EventType type;

  union EventData {
    struct InputData {
      uint32_t scancode;
    } input;
  } data;
};

class EventManager {
 public:
  void poll();

  [[nodiscard]] const std::vector<Event>& getEvents() const;

 private:
  std::vector<Event> m_events;

  void clear();
};

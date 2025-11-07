#pragma once
#include <cstdint>
#include <variant>
#include <vector>

enum class EventType : uint8_t {
  None,
  Quit,
  KeyDown,
  KeyUp,
  MouseButtonDown,
  MouseButtonUp,
  MouseMotion,
  MouseWheel
};

struct InputData {
  uint32_t scancode;
};

struct MotionData {
  float x;
  float y;
  float dx;
  float dy;
};

struct WheelData {
  float scroll;
};

struct Event {
  EventType type = EventType::None;
  std::variant<std::monostate, InputData, MotionData, WheelData> data;
};

class EventManager {
 public:
  void poll();

  [[nodiscard]] const std::vector<Event>& getEvents() const;

 private:
  std::vector<Event> m_events;

  static constexpr size_t expectedEvents = 64;

  void clear();
};

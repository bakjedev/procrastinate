#pragma once
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

enum class EventType : uint8_t
{
  kNone,
  kQuit,
  kKeyDown,
  kKeyUp,
  kMouseButtonDown,
  kMouseButtonUp,
  kMouseMotion,
  kMouseWheel,
  kWindowResized
};

struct InputData
{
  uint32_t scancode;
};

struct MotionData
{
  float x;
  float y;
  float dx;
  float dy;
};

struct WheelData
{
  float scroll;
};

struct WindowResizeData
{
  uint32_t width;
  uint32_t height;
};

struct Event
{
  EventType type = EventType::kNone;
  std::variant<std::monostate, InputData, MotionData, WheelData, WindowResizeData> data;
};

class EventManager
{
public:
  void poll();

  [[nodiscard]] const std::vector<Event>& getEvents() const;

private:
  std::vector<Event> events_;

  static constexpr size_t expected_events_ = 64;

  void clear();
};

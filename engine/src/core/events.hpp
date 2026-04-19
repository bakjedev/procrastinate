#pragma once
#include <cstdint>
#include <variant>
#include <vector>

union SDL_Event;

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
  using EventCallback = void (*)(SDL_Event*);

  void poll();

  [[nodiscard]] const std::vector<Event>& GetEvents() const;

  void AddCallback(const EventCallback& callback);

private:
  std::vector<Event> events_;
  static constexpr size_t expected_events_ = 64;

  std::vector<EventCallback> callbacks_;

  void clear();
};

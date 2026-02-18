#include "input.hpp"

#include "core/events.hpp"
#include "util/print.hpp"

Input::Input(EventManager& event_manager) : event_manager_(&event_manager) {}

void Input::update()
{
  mouse_scroll_ = 0.0F;
  mouse_delta_x_ = 0.0F;
  mouse_delta_y_ = 0.0F;

  for (const auto& event: event_manager_->getEvents())
  {
    switch (event.type)
    {
      case EventType::kKeyDown:
      {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(KeyboardKey::Count))
        {
          keys_down_.set(input.scancode);
        }
        break;
      }
      case EventType::kKeyUp:
      {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(KeyboardKey::Count))
        {
          keys_down_.reset(input.scancode);
        }
        break;
      }
      case EventType::kMouseButtonDown:
      {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(MouseButton::Count))
        {
          mouse_buttons_down_.set(input.scancode - 1);
        }
        break;
      }
      case EventType::kMouseButtonUp:
      {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(MouseButton::Count))
        {
          mouse_buttons_down_.reset(input.scancode - 1);
        }
        break;
      }
      case EventType::kMouseMotion:
      {
        const auto& motion = std::get<MotionData>(event.data);
        mouse_x_ = motion.x;
        mouse_y_ = motion.y;
        mouse_delta_x_ = motion.dx;
        mouse_delta_y_ = motion.dy;
        break;
      }
      case EventType::kMouseWheel:
      {
        const auto& wheel = std::get<WheelData>(event.data);
        mouse_scroll_ = wheel.scroll;
        break;
      }
      default:
        break;
    }
  }

  // update previous state
  keys_pressed_ = keys_down_ & ~keys_down_prev_;
  keys_released_ = ~keys_down_ & keys_down_prev_;
  keys_down_prev_ = keys_down_;

  mouse_buttons_pressed_ = mouse_buttons_down_ & ~mouse_buttons_down_prev_;
  mouse_buttons_released_ = ~mouse_buttons_down_ & mouse_buttons_down_prev_;
  mouse_buttons_down_prev_ = mouse_buttons_down_;
}

bool Input::KeyDown(KeyboardKey key) const
{
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count))
  {
    return false;
  }
  return keys_down_.test(index);
}

bool Input::KeyPressed(KeyboardKey key) const
{
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count))
  {
    return false;
  }
  return keys_pressed_.test(index);
}

bool Input::KeyReleased(KeyboardKey key) const
{
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count))
  {
    return false;
  }
  return keys_released_.test(index);
}

bool Input::MouseButtonDown(MouseButton button) const
{
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count))
  {
    return false;
  }
  return mouse_buttons_down_.test(index);
}

bool Input::MouseButtonPressed(MouseButton button) const
{
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count))
  {
    return false;
  }
  return mouse_buttons_pressed_.test(index);
}

bool Input::MouseButtonReleased(MouseButton button) const
{
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count))
  {
    return false;
  }
  return mouse_buttons_released_.test(index);
}

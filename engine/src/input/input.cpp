#include "input.hpp"

#include "core/events.hpp"

void Input::update(const EventManager& eventManager) {
  m_mouseScroll = 0.0F;
  m_mouseDeltaX = 0.0F;
  m_mouseDeltaY = 0.0F;

  for (const auto& event : eventManager.getEvents()) {
    switch (event.type) {
      case EventType::KeyDown: {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(KeyboardKey::Count)) {
          m_keysDown.set(input.scancode);
        }
        break;
      }
      case EventType::KeyUp: {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(KeyboardKey::Count)) {
          m_keysDown.reset(input.scancode);
        }
        break;
      }
      case EventType::MouseButtonDown: {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(MouseButton::Count)) {
          m_mouseButtonsDown.set(input.scancode - 1);
        }
        break;
      }
      case EventType::MouseButtonUp: {
        const auto& input = std::get<InputData>(event.data);
        if (input.scancode < static_cast<uint32_t>(MouseButton::Count)) {
          m_mouseButtonsDown.reset(input.scancode - 1);
        }
        break;
      }
      case EventType::MouseMotion: {
        const auto& motion = std::get<MotionData>(event.data);
        m_mouseX = motion.x;
        m_mouseY = motion.y;
        m_mouseDeltaX = motion.dx;
        m_mouseDeltaY = motion.dy;
        break;
      }
      case EventType::MouseWheel: {
        const auto& wheel = std::get<WheelData>(event.data);
        m_mouseScroll = wheel.scroll;
        break;
      }
      default:
        break;
    }
  }

  // update previous state
  m_keysPressed = m_keysDown & ~m_keysDownPrev;
  m_keysReleased = ~m_keysDown & m_keysDownPrev;
  m_keysDownPrev = m_keysDown;

  m_mouseButtonsPressed = m_mouseButtonsDown & ~m_mouseButtonsDownPrev;
  m_mouseButtonsReleased = ~m_mouseButtonsDown & m_mouseButtonsDownPrev;
  m_mouseButtonsDownPrev = m_mouseButtonsDown;
}

bool Input::keyDown(KeyboardKey key) const {
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count)) {
    return false;
  }
  return m_keysDown.test(index);
}

bool Input::keyPressed(KeyboardKey key) const {
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count)) {
    return false;
  }
  return m_keysPressed.test(index);
}

bool Input::keyReleased(KeyboardKey key) const {
  auto index = static_cast<size_t>(key);
  if (index >= static_cast<size_t>(KeyboardKey::Count)) {
    return false;
  }
  return m_keysReleased.test(index);
}

bool Input::mouseButtonDown(MouseButton button) const {
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count)) {
    return false;
  }
  return m_mouseButtonsDown.test(index);
}

bool Input::mouseButtonPressed(MouseButton button) const {
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count)) {
    return false;
  }
  return m_mouseButtonsPressed.test(index);
}

bool Input::mouseButtonReleased(MouseButton button) const {
  auto index = static_cast<size_t>(button);
  if (index >= static_cast<size_t>(MouseButton::Count)) {
    return false;
  }
  return m_mouseButtonsReleased.test(index);
}
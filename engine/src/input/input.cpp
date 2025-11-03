#include "input.hpp"

#include "core/events.hpp"

void Input::update(const EventManager& eventManager) {
  m_mouseScroll = 0.0F;

  for (const auto& event : eventManager.getEvents()) {
    switch (event.type) {
      case EventType::KeyDown:
        if (event.data.input.scancode <
            static_cast<uint32_t>(KeyboardKey::Count)) {
          m_keysDown.set(event.data.input.scancode);
        }
        break;
      case EventType::KeyUp:
        if (event.data.input.scancode <
            static_cast<uint32_t>(KeyboardKey::Count)) {
          m_keysDown.reset(event.data.input.scancode);
        }
        break;
      case EventType::MouseButtonDown:
        if (event.data.input.scancode <
            static_cast<uint32_t>(MouseButton::Count)) {
          m_mouseButtonsDown.set(event.data.input.scancode);
        }
        break;
      case EventType::MouseButtonUp:
        if (event.data.input.scancode <
            static_cast<uint32_t>(MouseButton::Count)) {
          m_mouseButtonsDown.reset(event.data.input.scancode);
        }
        break;
      case EventType::MouseMotion:
        m_mouseX = event.data.motion.x;
        m_mouseY = event.data.motion.y;
        m_mouseDeltaX = event.data.motion.dx;
        m_mouseDeltaY = event.data.motion.dy;
        break;
      case EventType::MouseWheel:
        m_mouseScroll = event.data.wheel.scroll;
        break;
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
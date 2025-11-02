#include "input.hpp"

#include <cstddef>

#include "SDL3/SDL_events.h"
#include "input_enums.hpp"

void Input::update(const EventManager& eventManager) {
  for (const auto& event : eventManager.getEvents()) {
    switch (event.type) {
      case SDL_EVENT_KEY_DOWN:
        m_keysDown.set(event.key.scancode);
        break;
      case SDL_EVENT_KEY_UP:
        m_keysDown.reset(event.key.scancode);
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        m_mouseButtonsDown.set(event.button.button);
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        m_mouseButtonsDown.reset(event.button.button);
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
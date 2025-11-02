#include "input.hpp"

void Input::update(const EventManager& eventManager) {
  for (const auto& event : eventManager.getEvents()) {
    switch (event.type) {
      case SDL_EVENT_KEY_DOWN:
        m_keysDown.set(event.key.scancode);
        break;
      case SDL_EVENT_KEY_UP:
        m_keysDown.reset(event.key.scancode);
        break;
      default:
        break;
    }
  }
}

void Input::updatePrev() {
  m_keysPressed = m_keysDown & ~m_keysDownPrev;
  m_keysReleased = ~m_keysDown & m_keysDownPrev;
  m_keysDownPrev = m_keysDown;
}

bool Input::keyDown(KeyboardKey key) const {
  return m_keysDown.test(static_cast<size_t>(key));
}

bool Input::keyPressed(KeyboardKey key) const {
  return m_keysPressed.test(static_cast<size_t>(key));
}

bool Input::keyReleased(KeyboardKey key) const {
  return m_keysReleased.test(static_cast<size_t>(key));
}

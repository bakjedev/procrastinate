#pragma once
#include <bitset>

#include "input_enums.hpp"

class EventManager;

class Input {
 public:
  void update(const EventManager& eventManager);

  [[nodiscard]] bool keyDown(KeyboardKey key) const;
  [[nodiscard]] bool keyPressed(KeyboardKey key) const;
  [[nodiscard]] bool keyReleased(KeyboardKey key) const;

  [[nodiscard]] bool mouseButtonDown(MouseButton button) const;
  [[nodiscard]] bool mouseButtonPressed(MouseButton button) const;
  [[nodiscard]] bool mouseButtonReleased(MouseButton button) const;

  [[nodiscard]] float getMouseX() const { return m_mouseX; }
  [[nodiscard]] float getMouseY() const { return m_mouseY; }
  [[nodiscard]] float getMouseDeltaX() const { return m_mouseDeltaX; }
  [[nodiscard]] float getMouseDeltaY() const { return m_mouseDeltaY; }
  [[nodiscard]] float getMouseScroll() const { return m_mouseScroll; }

 private:
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDown;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDownPrev;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysPressed;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysReleased;

  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsDown;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsDownPrev;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsPressed;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsReleased;

  float m_mouseX{0.0F};
  float m_mouseY{0.0F};
  float m_mouseDeltaX{0.0F};
  float m_mouseDeltaY{0.0F};
  float m_mouseScroll{0.0F};
};

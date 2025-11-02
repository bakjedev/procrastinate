#pragma once
#include <bitset>

#include "core/events.hpp"
#include "input_enums.hpp"

class Input {
 public:
  void update(const EventManager& eventManager);

  [[nodiscard]] bool keyDown(KeyboardKey key) const;
  [[nodiscard]] bool keyPressed(KeyboardKey key) const;
  [[nodiscard]] bool keyReleased(KeyboardKey key) const;

  [[nodiscard]] bool mouseButtonDown(MouseButton button) const;
  [[nodiscard]] bool mouseButtonPressed(MouseButton button) const;
  [[nodiscard]] bool mouseButtonReleased(MouseButton button) const;

 private:
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDown;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDownPrev;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysPressed;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysReleased;

  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsDown;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsDownPrev;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsPressed;
  std::bitset<static_cast<size_t>(MouseButton::Count)> m_mouseButtonsReleased;
};

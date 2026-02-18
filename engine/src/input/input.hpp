#pragma once
#include <bitset>

#include "input_enums.hpp"

class EventManager;

class Input
{
public:
  explicit Input(EventManager& event_manager);
  void update();

  [[nodiscard]] bool KeyDown(KeyboardKey key) const;
  [[nodiscard]] bool KeyPressed(KeyboardKey key) const;
  [[nodiscard]] bool KeyReleased(KeyboardKey key) const;

  [[nodiscard]] bool MouseButtonDown(MouseButton button) const;
  [[nodiscard]] bool MouseButtonPressed(MouseButton button) const;
  [[nodiscard]] bool MouseButtonReleased(MouseButton button) const;

  [[nodiscard]] float GetMouseX() const { return mouse_x_; }
  [[nodiscard]] float GetMouseY() const { return mouse_y_; }
  [[nodiscard]] float GetMouseDeltaX() const { return mouse_delta_x_; }
  [[nodiscard]] float GetMouseDeltaY() const { return mouse_delta_y_; }
  [[nodiscard]] float GetMouseScroll() const { return mouse_scroll_; }

private:
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> keys_down_;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> keys_down_prev_;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> keys_pressed_;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> keys_released_;

  std::bitset<static_cast<size_t>(MouseButton::Count)> mouse_buttons_down_;
  std::bitset<static_cast<size_t>(MouseButton::Count)> mouse_buttons_down_prev_;
  std::bitset<static_cast<size_t>(MouseButton::Count)> mouse_buttons_pressed_;
  std::bitset<static_cast<size_t>(MouseButton::Count)> mouse_buttons_released_;

  float mouse_x_{0.0F};
  float mouse_y_{0.0F};
  float mouse_delta_x_{0.0F};
  float mouse_delta_y_{0.0F};
  float mouse_scroll_{0.0F};

  EventManager* event_manager_;
};

#include <bitset>

#include "core/events.hpp"
#include "input_enums.hpp"

class Input {
 public:
  void update(const EventManager& eventManager);
  void updatePrev();

  [[nodiscard]] bool keyDown(KeyboardKey key) const;
  [[nodiscard]] bool keyPressed(KeyboardKey key) const;
  [[nodiscard]] bool keyReleased(KeyboardKey key) const;

 private:
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDown;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysDownPrev;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysPressed;
  std::bitset<static_cast<size_t>(KeyboardKey::Count)> m_keysReleased;
};

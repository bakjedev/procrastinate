#include "core/engine.hpp"
#include "core/events.hpp"
#include "core/window.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "util/util.hpp"

int main() {
  Engine engine;
  Util::print("forty five {}\n", 45);

  auto& event_manager = engine.getEventManager();
  auto& window = engine.getWindow();
  auto& input = engine.getInput();

  while (!window.shouldQuit()) {
    event_manager.poll();
    window.update(event_manager);
    input.update(event_manager);

    if (input.mouseButtonReleased(MouseButton::Left)) {
      Util::println("RELEASED");
    }
    if (input.mouseButtonPressed(MouseButton::Right)) {
      Util::println("PRESSED {} {}", input.getMouseX(), input.getMouseY());
    }
    if (input.mouseButtonDown(MouseButton::Middle)) {
      Util::println("DOWN");
    }

    if (input.getMouseScroll() != 0.0F) {
      Util::println("TESTING {}", input.getMouseScroll());
    }
  }
  return 0;
}
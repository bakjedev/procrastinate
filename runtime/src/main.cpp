#include "core/engine.hpp"
#include "core/events.hpp"
#include "core/window.hpp"
#include "input/input.hpp"
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

    if (input.keyDown(KeyboardKey::Space)) {
      Util::println("ASDASD");
    }

    input.updatePrev();
  }
  return 0;
}
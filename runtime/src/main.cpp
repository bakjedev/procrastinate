#include "core/engine.hpp"
#include "core/events.hpp"
#include "core/window.hpp"
#include "util/util.hpp"

int main() {
  Engine engine;
  Util::print("forty five {}\n", 45);

  auto& event_manager = engine.getEventManager();
  auto& window = engine.getWindow();

  while (!window.shouldQuit()) {
    event_manager.poll();
    window.update(event_manager);
  }
  return 0;
}
#include "core/engine.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "util/print.hpp"

struct RuntimeApplication {
  void init() {}

  void update(float) const {
    auto& input = engine->getInput();

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

  void fixedUpdate(float) {}
  void render() {}
  void shutdown() {}

  Engine* engine = nullptr;
};

int main() {
  Engine engine;

  RuntimeApplication app{&engine};

  engine.run(app);

  return 0;
}
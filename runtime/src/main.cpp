#include "core/engine.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource.hpp"
#include "util/util.hpp"

struct TestResource {
  int number = 44;
};

struct TestResourceLoader {
  TestResource operator()(int value) const { return TestResource{value}; }
};

struct RuntimeApplication {
  void init() {
    ResourcePool<TestResource> pool;

    auto test = pool.load("test", TestResourceLoader{}, 22);

    Util::println("test {}", test->number);

    auto testTWO = pool.load("test", TestResourceLoader{}, 16);

    Util::println("testTWO {}", testTWO->number);
  }

  void update(float) {
    auto& input = engine.getInput();

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

  Engine& engine;
};

int main() {
  Engine engine;

  RuntimeApplication app{engine};

  engine.run(app);

  return 0;
}
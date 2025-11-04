#include "core/engine.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource_pool.hpp"
#include "util/util.hpp"

struct TestResource {
  int number = 44;

  TestResource() { Util::println("CREATED TEST RESOURCE"); }
  TestResource(int value) : number(value) {
    Util::println("CREATED TEST RESOURCE with {}", number);
  }

  ~TestResource() { Util::println("DESTROYED TEST RESOURCE"); }
};

struct TestResourceLoader {
  TestResource operator()(int value) const { return TestResource{value}; }
};

struct RuntimeApplication {
  void init() {
    ResourcePool<TestResource> pool;
    auto handle = pool.allocate();
    pool.registerKey("test", handle.index);
    pool.load(handle, TestResourceLoader{}, 37);
    pool.release(handle);
    Util::print("forty five {}\n", 45);
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
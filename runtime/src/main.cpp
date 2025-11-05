#include "core/engine.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource.hpp"
#include "util/util.hpp"

struct Data {
  uint32_t number;
  Data() : number(1) { Util::println("Default constructor: {}", number); }

  Data(uint32_t n) : number(n) {
    Util::println("Parameterized constructor: {}", number);
  }

  Data(const Data& other) : number(other.number) {
    Util::println("Copy constructor: {}", number);
  }

  Data(Data&& other) noexcept : number(std::move(other.number)) {
    Util::println("Move constructor: {}", number);
    other.number = 0;
  }

  Data& operator=(const Data& other) {
    if (this != &other) {
      number = other.number;
      Util::println("Copy assignment: {}", number);
    }
    return *this;
  }

  Data& operator=(Data&& other) noexcept {
    if (this != &other) {
      number = std::move(other.number);
      Util::println("Move assignment: {}", number);
      other.number = 0;
    }
    return *this;
  }

  ~Data() { Util::println("Destructor: {}", number); }
};

struct RuntimeApplication {
  void init() {
    ResourceStorage<Data> storage;

    auto loader = [](uint32_t number) { return Data{number}; };

    auto handleA = storage.create("A", loader, 3);

    // storage.destroy(handleA);

    auto handleB = storage.create("A", loader, 6);

    // storage.destroy(handleA);
    storage.destroy(handleB);

    auto dataA = storage.get(handleA);
    auto dataB = storage.get(handleB);

    if (dataA) {
      Util::println("A {}", dataA->number);
    } else {
      Util::println("A fake");
    }

    if (dataB) {
      Util::println("B {}", dataB->number);
    } else {
      Util::println("B fake");
    }
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
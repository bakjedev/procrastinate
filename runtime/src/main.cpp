#include "core/engine.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/scene.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "util/print.hpp"

struct RuntimeApplication {
  void init(Engine& eng) {
    engine = &eng;

    mesh_entity = engine->getScene().create();

    engine->getScene().addComponent<MeshComponent>(mesh_entity);
  }

  void update(float /*unused*/) const {
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

  void fixedUpdate(float /*unused*/) {}
  void render() {}
  void shutdown() const { engine->getScene().destroy(mesh_entity); }

  Engine* engine = nullptr;
  uint32_t mesh_entity{};
};

int main() {
  Engine engine;

  RuntimeApplication app{};

  engine.run(app);

  return 0;
}
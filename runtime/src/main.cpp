#include "core/engine.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/scene.hpp"
#include "files/files.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/mesh_resource.hpp"
#include "util/print.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct RuntimeApplication {
  void init(Engine &eng) {
    engine = &eng;

    auto rootPath = Files::getResourceRoot();
    constexpr int gridSize = 30;
    for (int j{}; j < gridSize; ++j) {
      for (int i{}; i < gridSize; ++i) {
        const auto entity = engine->getScene().create();
        auto* transformComponent = engine->getScene().addComponent<CTransform>(entity);
        auto* meshComponent = engine->getScene().addComponent<CMesh>(entity);

        constexpr float spacing = 2.6F;
        transformComponent->world = glm::translate(glm::mat4(1.0F),
                                                   glm::vec3(static_cast<float>(j) * spacing,
                                                             0.0F,
                                                             static_cast<float>(i) * spacing));
        transformComponent->world = glm::translate(transformComponent->world, glm::vec3(-static_cast<float>(gridSize), 6.0F, 20.0F));
        meshComponent->mesh = engine->getResourceManager().create<MeshResource>(
            "firstmesh", MeshResourceLoader{}, (rootPath / "engine/assets/cylinder.obj").string(),
            engine->getRenderer());
      }
    }
  }

  void update(float /*unused*/) const {
    auto &input = engine->getInput();

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

    if (input.keyDown(KeyboardKey::A)) {
      engine->getWindow().quit();
    }
  }

  void fixedUpdate(float /*unused*/) {}
  void render() {}
  void shutdown() const {}

  Engine *engine = nullptr;
};

int main() {
  Engine engine;

  RuntimeApplication app{};

  engine.run(app);

  return 0;
}

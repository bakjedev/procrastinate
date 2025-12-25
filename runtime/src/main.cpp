#include "core/engine.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/scene.hpp"
#include "files/files.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/mesh_resource.hpp"
#include "util/print.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct RuntimeApplication {
  void init(Engine &eng) {
    engine = &eng;

    mesh_entity = engine->getScene().create();

    auto rootPath = Files::getResourceRoot();

    auto *comp = engine->getScene().addComponent<CMesh>(mesh_entity);
    comp->mesh = engine->getResourceManager().create<MeshResource>(
        "firstmesh", MeshResourceLoader{}, (rootPath / "engine/assets/cylinder.obj").string(),
        engine->getRenderer());
    auto *transformComp = engine->getScene().addComponent<CTransform>(mesh_entity);
    transformComp->world = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 3.0f));

    mesh_entity2 = engine->getScene().create();

    auto *comp2 = engine->getScene().addComponent<CMesh>(mesh_entity2);
    comp2->mesh = engine->getResourceManager().create<MeshResource>(
        "firstmesh", MeshResourceLoader{}, (rootPath / "engine/assets/cylinder.obj").string(),
        engine->getRenderer());
    auto *transformComp2 = engine->getScene().addComponent<CTransform>(mesh_entity2);
    transformComp2->world = glm::mat4(1.0f);
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
  void shutdown() const { engine->getScene().destroy(mesh_entity); }

  Engine *engine = nullptr;
  uint32_t mesh_entity{};
  uint32_t mesh_entity2{};
};

int main() {
  Engine engine;

  RuntimeApplication app{};

  engine.run(app);

  return 0;
}

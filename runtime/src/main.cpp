#include "core/engine.hpp"
#include "ecs/components/mesh_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/scene.hpp"
#include "files/files.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "input/input.hpp"
#include "input/input_enums.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/mesh_resource.hpp"
#include "util/print.hpp"

struct RuntimeApplication {
  void init(Engine& eng) {
    engine = &eng;

    cameraEntity = engine->getScene().create();
    auto* cameraTransform =
        engine->getScene().addComponent<CTransform>(cameraEntity);
    auto* cameraComponent =
        engine->getScene().addComponent<CCamera>(cameraEntity);
    cameraTransform->world = glm::mat4(1.0F);
    cameraComponent->fov = 70.0F;

    auto rootPath = Files::getResourceRoot();
    constexpr int gridSize = 100;
    for (int j{}; j < gridSize; ++j) {
      for (int i{}; i < gridSize; ++i) {
        const auto entity = engine->getScene().create();
        auto* transformComponent =
            engine->getScene().addComponent<CTransform>(entity);
        auto* meshComponent = engine->getScene().addComponent<CMesh>(entity);

        constexpr float spacing = 2.6F;
        transformComponent->world = glm::translate(
            glm::mat4(1.0F), glm::vec3(static_cast<float>(j) * spacing, 0.0F,
                                       static_cast<float>(i) * spacing));
        transformComponent->world = glm::translate(
            transformComponent->world,
            glm::vec3(-static_cast<float>(gridSize), 6.0F, -100.0F));
        transformComponent->world =
            glm::scale(transformComponent->world, glm::vec3(1.0F));

        if ((i + j) % 2 == 0) {
          meshComponent->mesh =
              engine->getResourceManager().create<MeshResource>(
                  "firstmesh", MeshResourceLoader{},
                  (rootPath / "engine/assets/cylinder.obj").string(),
                  engine->getRenderer());
        } else {
          meshComponent->mesh =
              engine->getResourceManager().create<MeshResource>(
                  "secondmesh", MeshResourceLoader{},
                  (rootPath / "engine/assets/icosphere.obj").string(),
                  engine->getRenderer());
        }
      }
    }
  }

  void update(float deltaTime) const {
    auto& renderer = engine->getRenderer();
    auto& input = engine->getInput();

    renderer.renderLine(glm::vec3(-1.0F, 3.0F, 0.0F), glm::vec3(1.0F, 3.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

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

    if (input.keyDown(KeyboardKey::Escape)) {
      engine->getWindow().quit();
    }

    auto& transform = engine->getScene().registry().get<CTransform>(
        static_cast<entt::entity>(cameraEntity));
    constexpr float cameraSpeed = 50.0F;
    if (input.keyDown(KeyboardKey::W)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(0.0F, 0.0F, -deltaTime * cameraSpeed));
    } else if (input.keyDown(KeyboardKey::S)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(0.0F, 0.0F, deltaTime * cameraSpeed));
    }
    if (input.keyDown(KeyboardKey::A)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(-deltaTime * cameraSpeed, 0.0F, 0.0F));
    } else if (input.keyDown(KeyboardKey::D)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(deltaTime * cameraSpeed, 0.0F, 0.0F));
    }
    if (input.keyDown(KeyboardKey::Space)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(0.0F, -deltaTime * cameraSpeed, 0.0F));
    } else if (input.keyDown(KeyboardKey::LeftControl)) {
      transform.world = glm::translate(
          transform.world, glm::vec3(0.0F, deltaTime * cameraSpeed, 0.0F));
    }
  }

  void fixedUpdate(float /*unused*/) {}
  void render() {}
  void shutdown() const {}

  uint32_t cameraEntity;

  Engine* engine = nullptr;
};

int main() {
  try {
    Engine engine;

    RuntimeApplication app{};

    engine.run(app);
  } catch (const std::exception& err) {
    Util::println("Fatal error: {}", err.what());
    return 1;
  }
  return 0;
}

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

struct RuntimeApplication
{
  void init(Engine& eng)
  {
    engine = &eng;
    auto rootPath = files::GetAssetsPathRoot();

    const auto catEntity = engine->GetScene().Create();
    auto* catTransform = engine->GetScene().AddComponent<CTransform>(catEntity);
    auto* catMesh = engine->GetScene().AddComponent<CMesh>(catEntity);
    catMesh->mesh = engine->GetResourceManager().create<MeshResource>(
        "catMesh", MeshResourceLoader{}, (rootPath / "engine/assets/concrete_cat_statue_1k.obj").string(), engine);
    catTransform->world = glm::mat4(1.0F);
    catTransform->world = glm::translate(catTransform->world, glm::vec3(0.0F, -20.0F, 0.0F));
    catTransform->world = glm::scale(catTransform->world, glm::vec3(10.0F, 10.0F, 10.0F));
    catTransform->world = glm::rotate(catTransform->world, glm::radians(180.0F), glm::vec3(1.0F, 0.0F, 0.0F));


    const auto wallEntity = engine->GetScene().Create();
    auto* wallTransform = engine->GetScene().AddComponent<CTransform>(wallEntity);
    auto* wallMesh = engine->GetScene().AddComponent<CMesh>(wallEntity);
    wallMesh->mesh = engine->GetResourceManager().create<MeshResource>(
        "wallMesh", MeshResourceLoader{}, (rootPath / "engine/assets/wall.obj").string(), engine);
    wallTransform->world = glm::mat4(1.0F);
    wallTransform->world = glm::translate(wallTransform->world, glm::vec3(30.0F, 0.0F, 180.0F));
    wallTransform->world = glm::scale(wallTransform->world, glm::vec3(1.2F, 1.0F, 1.0F));

    cameraEntity = engine->GetScene().Create();
    auto* cameraTransform = engine->GetScene().AddComponent<CTransform>(cameraEntity);
    auto* cameraComponent = engine->GetScene().AddComponent<CCamera>(cameraEntity);
    cameraTransform->world = glm::mat4(1.0F);
    cameraComponent->fov = 70.0F;

    constexpr int gridSize = 100;
    for (int j{}; j < gridSize; ++j)
    {
      for (int i{}; i < gridSize; ++i)
      {
        const auto entity = engine->GetScene().Create();
        auto* transformComponent = engine->GetScene().AddComponent<CTransform>(entity);
        auto* meshComponent = engine->GetScene().AddComponent<CMesh>(entity);

        constexpr float spacing = 2.6F;
        transformComponent->world = glm::translate(
            glm::mat4(1.0F), glm::vec3(static_cast<float>(j) * spacing, 0.0F, static_cast<float>(i) * spacing));
        transformComponent->world =
            glm::translate(transformComponent->world, glm::vec3(-static_cast<float>(gridSize), 6.0F, -100.0F));
        transformComponent->world = glm::scale(transformComponent->world, glm::vec3(1.0F));

        if ((i + j) % 2 == 0)
        {
          meshComponent->mesh = engine->GetResourceManager().create<MeshResource>(
              "firstmesh", MeshResourceLoader{}, (rootPath / "engine/assets/cylinder.obj").string(), engine);
        } else
        {
          meshComponent->mesh = engine->GetResourceManager().create<MeshResource>(
              "secondmesh", MeshResourceLoader{}, (rootPath / "engine/assets/icosphere.obj").string(), engine);
        }
      }
    }
  }

  void update(float deltaTime) const
  {
    auto& renderer = engine->GetRenderer();
    auto& input = engine->GetInput();

    // H
    renderer.RenderLine(glm::vec3(0.0F, -5.0F, 0.0F), glm::vec3(0.0F, -8.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(0.0F, -6.5F, 0.0F), glm::vec3(2.0F, -6.5F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(2.0F, -5.0F, 0.0F), glm::vec3(2.0F, -8.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // E
    renderer.RenderLine(glm::vec3(3.0F, -5.0F, 0.0F), glm::vec3(4.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(3.0F, -6.0F, 0.0F), glm::vec3(4.0F, -6.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(3.0F, -7.0F, 0.0F), glm::vec3(4.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(3.0F, -5.0F, 0.0F), glm::vec3(3.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // L
    renderer.RenderLine(glm::vec3(5.0F, -5.0F, 0.0F), glm::vec3(5.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(5.0F, -5.0F, 0.0F), glm::vec3(6.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // L
    renderer.RenderLine(glm::vec3(7.0F, -5.0F, 0.0F), glm::vec3(7.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(7.0F, -5.0F, 0.0F), glm::vec3(8.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // O
    renderer.RenderLine(glm::vec3(9.0F, -5.0F, 0.0F), glm::vec3(11.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(9.0F, -7.0F, 0.0F), glm::vec3(11.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(9.0F, -5.0F, 0.0F), glm::vec3(9.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(11.0F, -5.0F, 0.0F), glm::vec3(11.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // W
    renderer.RenderLine(glm::vec3(13.0F, -7.0F, 0.0F), glm::vec3(14.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(14.0F, -5.0F, 0.0F), glm::vec3(15.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(15.0F, -7.0F, 0.0F), glm::vec3(16.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(16.0F, -5.0F, 0.0F), glm::vec3(17.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // O
    renderer.RenderLine(glm::vec3(18.0F, -5.0F, 0.0F), glm::vec3(20.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(18.0F, -7.0F, 0.0F), glm::vec3(20.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(18.0F, -5.0F, 0.0F), glm::vec3(18.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(20.0F, -5.0F, 0.0F), glm::vec3(20.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // R
    renderer.RenderLine(glm::vec3(21.0F, -5.0F, 0.0F), glm::vec3(21.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(21.0F, -7.0F, 0.0F), glm::vec3(22.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // L
    renderer.RenderLine(glm::vec3(23.0F, -5.0F, 0.0F), glm::vec3(23.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(23.0F, -5.0F, 0.0F), glm::vec3(24.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));

    // D
    renderer.RenderLine(glm::vec3(25.0F, -5.0F, 0.0F), glm::vec3(25.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(25.0F, -5.0F, 0.0F), glm::vec3(26.0F, -5.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(26.0F, -5.0F, 0.0F), glm::vec3(26.5F, -6.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(25.0F, -7.0F, 0.0F), glm::vec3(26.0F, -7.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    renderer.RenderLine(glm::vec3(26.0F, -7.0F, 0.0F), glm::vec3(26.5F, -6.0F, 0.0F), glm::vec3(1.0F, 0.5F, 0.0F));
    if (input.MouseButtonReleased(MouseButton::Left))
    {
      util::println("RELEASED");
    }
    if (input.MouseButtonPressed(MouseButton::Right))
    {
      util::println("PRESSED {} {}", input.GetMouseX(), input.GetMouseY());
    }
    if (input.MouseButtonDown(MouseButton::Middle))
    {
      util::println("DOWN");
    }
    if (input.GetMouseScroll() != 0.0F)
    {
      util::println("TESTING {}", input.GetMouseScroll());
    }

    if (input.KeyDown(KeyboardKey::Escape))
    {
      engine->GetWindow().quit();
    }

    auto& transform = engine->GetScene().registry().get<CTransform>(static_cast<entt::entity>(cameraEntity));
    constexpr float cameraSpeed = 50.0F;
    if (input.KeyDown(KeyboardKey::W))
    {
      transform.world = glm::translate(transform.world, glm::vec3(0.0F, 0.0F, -deltaTime * cameraSpeed));
    } else if (input.KeyDown(KeyboardKey::S))
    {
      transform.world = glm::translate(transform.world, glm::vec3(0.0F, 0.0F, deltaTime * cameraSpeed));
    }
    if (input.KeyDown(KeyboardKey::A))
    {
      transform.world = glm::translate(transform.world, glm::vec3(-deltaTime * cameraSpeed, 0.0F, 0.0F));
    } else if (input.KeyDown(KeyboardKey::D))
    {
      transform.world = glm::translate(transform.world, glm::vec3(deltaTime * cameraSpeed, 0.0F, 0.0F));
    }
    if (input.KeyDown(KeyboardKey::Space))
    {
      transform.world = glm::translate(transform.world, glm::vec3(0.0F, -deltaTime * cameraSpeed, 0.0F));
    } else if (input.KeyDown(KeyboardKey::LeftControl))
    {
      transform.world = glm::translate(transform.world, glm::vec3(0.0F, deltaTime * cameraSpeed, 0.0F));
    }

    if (input.KeyDown(KeyboardKey::Left))
    {
      transform.world = glm::rotate(transform.world, deltaTime, glm::vec3(0.0F, 1.0F, 0.0F));
    } else if (input.KeyDown(KeyboardKey::Right))
    {
      transform.world = glm::rotate(transform.world, deltaTime, glm::vec3(0.0F, -1.0F, 0.0F));
    }
  }

  void fixedUpdate(float /*unused*/) {}
  void render() {}
  void shutdown() const {}

  uint32_t cameraEntity;

  Engine* engine = nullptr;
};

int main()
{
  try
  {
    Engine engine;

    RuntimeApplication app{};

    engine.run(app);
  } catch (const std::exception& err)
  {
    util::println("Fatal error: {}", err.what());
    return 1;
  }
  return 0;
}

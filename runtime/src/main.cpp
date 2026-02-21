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
  void Init(Engine& eng)
  {
    engine = &eng;
    const auto& root_path = files::GetAssetsPathRoot();

    const auto cat_entity = engine->GetScene().Create();
    auto* cat_transform = engine->GetScene().AddComponent<CTransform>(cat_entity);
    auto* cat_mesh = engine->GetScene().AddComponent<CMesh>(cat_entity);
    cat_mesh->mesh = engine->GetResourceManager().create<MeshResource>(
        "catMesh", MeshResourceLoader{}, (root_path / "engine/assets/concrete_cat_statue_1k.obj").string(), engine);
    cat_transform->world = glm::mat4(1.0F);
    cat_transform->world = glm::translate(cat_transform->world, glm::vec3(0.0F, -20.0F, 0.0F));
    cat_transform->world = glm::scale(cat_transform->world, glm::vec3(10.0F, 10.0F, 10.0F));
    cat_transform->world = glm::rotate(cat_transform->world, glm::radians(180.0F), glm::vec3(1.0F, 0.0F, 0.0F));


    const auto wall_entity = engine->GetScene().Create();
    auto* wall_transform = engine->GetScene().AddComponent<CTransform>(wall_entity);
    auto* wall_mesh = engine->GetScene().AddComponent<CMesh>(wall_entity);
    wall_mesh->mesh = engine->GetResourceManager().create<MeshResource>(
        "wallMesh", MeshResourceLoader{}, (root_path / "engine/assets/wall.obj").string(), engine);
    wall_transform->world = glm::mat4(1.0F);
    wall_transform->world = glm::translate(wall_transform->world, glm::vec3(30.0F, 0.0F, 180.0F));
    wall_transform->world = glm::scale(wall_transform->world, glm::vec3(1.2F, 1.0F, 1.0F));

    camera_entity = engine->GetScene().Create();
    auto* camera_transform = engine->GetScene().AddComponent<CTransform>(camera_entity);
    auto* camera_component = engine->GetScene().AddComponent<CCamera>(camera_entity);
    camera_transform->world = glm::mat4(1.0F);
    camera_component->fov = 70.0F;

    constexpr int grid_size = 100;
    for (int j{}; j < grid_size; ++j)
    {
      for (int i{}; i < grid_size; ++i)
      {
        const auto entity = engine->GetScene().Create();
        auto* transform_component = engine->GetScene().AddComponent<CTransform>(entity);
        auto* mesh_component = engine->GetScene().AddComponent<CMesh>(entity);

        constexpr float spacing = 2.6F;
        transform_component->world = glm::translate(
            glm::mat4(1.0F), glm::vec3(static_cast<float>(j) * spacing, 0.0F, static_cast<float>(i) * spacing));
        transform_component->world =
            glm::translate(transform_component->world, glm::vec3(-static_cast<float>(grid_size), 6.0F, -100.0F));
        transform_component->world = glm::scale(transform_component->world, glm::vec3(1.0F));

        if ((i + j) % 2 == 0)
        {
          mesh_component->mesh = engine->GetResourceManager().create<MeshResource>(
              "firstmesh", MeshResourceLoader{}, (root_path / "engine/assets/cylinder.obj").string(), engine);
        } else
        {
          mesh_component->mesh = engine->GetResourceManager().create<MeshResource>(
              "secondmesh", MeshResourceLoader{}, (root_path / "engine/assets/icosphere.obj").string(), engine);
        }
      }
    }
  }

  void Update(const float delta_time) const
  {
    auto& renderer = engine->GetRenderer();
    const auto& input = engine->GetInput();

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

    auto& [camera_world] = engine->GetScene().registry().get<CTransform>(static_cast<entt::entity>(camera_entity));
    constexpr float camera_speed = 50.0F;
    if (input.KeyDown(KeyboardKey::W))
    {
      camera_world = glm::translate(camera_world, glm::vec3(0.0F, 0.0F, -delta_time * camera_speed));
    } else if (input.KeyDown(KeyboardKey::S))
    {
      camera_world = glm::translate(camera_world, glm::vec3(0.0F, 0.0F, delta_time * camera_speed));
    }
    if (input.KeyDown(KeyboardKey::A))
    {
      camera_world = glm::translate(camera_world, glm::vec3(-delta_time * camera_speed, 0.0F, 0.0F));
    } else if (input.KeyDown(KeyboardKey::D))
    {
      camera_world = glm::translate(camera_world, glm::vec3(delta_time * camera_speed, 0.0F, 0.0F));
    }
    if (input.KeyDown(KeyboardKey::Space))
    {
      camera_world = glm::translate(camera_world, glm::vec3(0.0F, -delta_time * camera_speed, 0.0F));
    } else if (input.KeyDown(KeyboardKey::LeftControl))
    {
      camera_world = glm::translate(camera_world, glm::vec3(0.0F, delta_time * camera_speed, 0.0F));
    }

    if (input.KeyDown(KeyboardKey::Left))
    {
      camera_world = glm::rotate(camera_world, delta_time, glm::vec3(0.0F, 1.0F, 0.0F));
    } else if (input.KeyDown(KeyboardKey::Right))
    {
      camera_world = glm::rotate(camera_world, delta_time, glm::vec3(0.0F, -1.0F, 0.0F));
    }
  }

  void FixedUpdate(float /*unused*/) {}
  void Render() {}
  void Shutdown() const {}

  uint32_t camera_entity;

  Engine* engine = nullptr;
};

int main()
{
  try
  {
    Engine engine;

    RuntimeApplication app{};

    engine.Run(app);
  } catch (const std::exception& err)
  {
    util::println("Fatal error: {}", err.what());
    return 1;
  }
  return 0;
}

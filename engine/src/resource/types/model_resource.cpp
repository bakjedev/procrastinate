#include "model_resource.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <stack>

#include "core/engine.hpp"
#include "glm/gtc/quaternion.hpp"
#include "render/vk_renderer.hpp"

ModelResource ModelResourceLoader::operator()(const std::string &path, Engine *engine) const
{
  ModelResource result;

  fastgltf::Parser parser;
  auto data = fastgltf::GltfDataBuffer::FromPath(path);
  if (data.error() != fastgltf::Error::None)
  {
    throw std::runtime_error("Failed to load gltf file: " + path);
  }

  auto asset =
      parser.loadGltfBinary(data.get(), path.substr(0, path.rfind('/')), fastgltf::Options::LoadExternalBuffers);
  if (asset.error() != fastgltf::Error::None)
  {
    throw std::runtime_error("Failed to load gltf: " + path);
  }

  const auto &loaded_asset = asset.get();

  for (const auto &material: loaded_asset.materials)
  {
  }

  uint32_t primitive_id = 0;
  std::stack<std::pair<const fastgltf::Node *, glm::mat4>> node_stack;

  for (const auto node_index: loaded_asset.scenes[0].nodeIndices)
  {
    const auto *node = &loaded_asset.nodes[node_index];
    node_stack.emplace(node, glm::mat4(1.0F));
  }

  while (!node_stack.empty())
  {
    const auto [node, parent_global_transform] = node_stack.top();
    node_stack.pop();

    const auto local_transform = GetNodeTransform(*node);
    const auto global_transform = parent_global_transform * local_transform;

    for (const auto child_node_index: node->children)
    {
      node_stack.emplace(&loaded_asset.nodes[child_node_index], global_transform);
    }

    if (node->meshIndex.has_value())
    {
      const auto &mesh = loaded_asset.meshes[node->meshIndex.value()];
      for (const auto &primitive: mesh.primitives)
      {
        auto vertices = GetVerticesFromPrimitives(loaded_asset, primitive);
        auto indices = GetIndicesFromPrimitives(loaded_asset, primitive);

        auto &resource_manager = engine->GetResourceManager();

        auto mesh_resource =
            resource_manager.load<MeshResource>(path + std::to_string(primitive_id), MeshDataResourceLoader{},
                                                std::move(indices), std::move(vertices), engine);

        primitive_id += 1;

        result.nodes.push_back({.mesh = mesh_resource, .transform = global_transform});
      }
    }
  }
  return result;
}

std::vector<Vertex> ModelResourceLoader::GetVerticesFromPrimitives(const fastgltf::Asset &model,
                                                                   const fastgltf::Primitive &primitive)
{
  std::vector<glm::vec3> positions;
  auto &position_accessor = model.accessors[primitive.findAttribute("POSITION")->accessorIndex];
  positions.resize(position_accessor.count);
  fastgltf::iterateAccessorWithIndex<glm::vec3>(
      model, position_accessor, [&](const glm::vec3 position, const std::size_t idx) { positions[idx] = position; });

  std::vector<Vertex> vertices;
  vertices.resize(positions.size());

  for (size_t i = 0; i < positions.size(); i++)
  {
    vertices[i] = {.position = positions[i], .color = {}, .normal = {}, .tex_coord = {}};
  }

  return vertices;
}
std::vector<uint32_t> ModelResourceLoader::GetIndicesFromPrimitives(const fastgltf::Asset &model,
                                                                    const fastgltf::Primitive &primitive)
{
  auto indices = std::vector<uint32_t>();
  auto &accessor = model.accessors[primitive.indicesAccessor.value()];
  indices.resize(accessor.count);
  fastgltf::iterateAccessorWithIndex<uint32_t>(model, accessor,
                                               [&](const uint32_t index, const size_t idx) { indices[idx] = index; });
  return indices;
}

glm::mat4 ModelResourceLoader::GetNodeTransform(const fastgltf::Node &node)
{
  glm::mat4 transform{1.0F};

  if (const auto *trs = std::get_if<fastgltf::TRS>(&node.transform))
  {
    const auto rotation = glm::quat{trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]};
    const auto scale = glm::vec3{trs->scale[0], trs->scale[1], trs->scale[2]};
    const auto translation = glm::vec3{trs->translation[0], trs->translation[1], trs->translation[2]};

    const glm::mat4 rotation_mat = glm::mat4_cast(rotation);

    transform = glm::scale(glm::translate(transform, translation) * rotation_mat, scale);
  } else if (const auto *mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform))
  {
    const auto &matrix = *mat;
    transform = {matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3], matrix[1][0], matrix[1][1],
                 matrix[1][2], matrix[1][3], matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
                 matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]};
  }

  return transform;
}

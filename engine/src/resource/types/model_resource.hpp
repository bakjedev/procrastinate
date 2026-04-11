#pragma once
#include <cstdint>
#include "glm/mat4x4.hpp"
#include "mesh_resource.hpp"
#include "resource/resource.hpp"

namespace fastgltf
{
  struct Node;
  struct Primitive;
  class Asset;
} // namespace fastgltf

struct Vertex;

struct ModelResource
{
  struct Node
  {
    ResourceHandle<MeshResource> mesh;
    glm::mat4 transform;
  };

  std::vector<Node> nodes;
};

struct ModelResourceLoader
{
  ModelResource operator()(const std::string& path, Engine* engine) const;

  static std::vector<Vertex> GetVerticesFromPrimitives(const fastgltf::Asset& model,
                                                       const fastgltf::Primitive& primitive);

  static std::vector<uint32_t> GetIndicesFromPrimitives(const fastgltf::Asset& model,
                                                        const fastgltf::Primitive& primitive);

  static glm::mat4 GetNodeTransform(const fastgltf::Node& node);
};

#include "mesh_resource.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

MeshResource MeshResourceLoader::operator()(const std::string &path,
                                            VulkanRenderer &renderer) const {
  MeshResource res{};
  std::vector<tinyobj::shape_t> shapes;
  tinyobj::attrib_t attrib;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  std::string err;
  std::string warn;

  if (!tinyobj::LoadObj(&attrib, &shapes, nullptr, &warn, &err, path.c_str())) {
    throw std::runtime_error(warn + err);
  }

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {

      const glm::vec3 pos = {attrib.vertices[index.vertex_index * 3],
                       attrib.vertices[(index.vertex_index * 3) + 1],
                       attrib.vertices[(index.vertex_index * 3) + 2]};

      const glm::vec3 col = {attrib.colors[index.vertex_index * 3],
                       attrib.colors[(index.vertex_index * 3) + 1],
                       attrib.colors[(index.vertex_index * 3) + 2]};

      auto nor = glm::vec3(0.0F);
      if (!attrib.normals.empty()) {
        nor = {attrib.normals[index.normal_index * 3],
                       attrib.normals[(index.normal_index * 3) + 1],
                       attrib.normals[(index.normal_index * 3) + 2]};
      }

      vertices.emplace_back(pos, col, nor);
      indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
    }
  }

  res.renderer_id = renderer.addMesh(vertices, indices, renderer.getIndexCount(), renderer.getVertexCount());
  renderer.upload();

  return res;
}
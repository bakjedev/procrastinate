#pragma once
#include <tinyobjloader/tiny_obj_loader.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "render/vk_renderer.hpp"

struct MeshResource {
  uint32_t startIndex;
  uint32_t indexCount;
  uint32_t startVertex;
  uint32_t vertexCount;
};

struct MeshResourceLoader {
  MeshResource operator()(const std::string& path) const {
    MeshResource res{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          path.c_str())) {
      throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
      for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertex.x = attrib.vertices[(3 * index.vertex_index) + 0];
        vertex.y = attrib.vertices[(3 * index.vertex_index) + 1];
        vertex.z = attrib.vertices[(3 * index.vertex_index) + 2];

        if (!uniqueVertices.contains(vertex)) {
          uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
          vertices.push_back(vertex);
        }
        indices.push_back(uniqueVertices[vertex]);
      }
    }
    return res;
  }
};
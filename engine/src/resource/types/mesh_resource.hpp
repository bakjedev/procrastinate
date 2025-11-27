#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "render/vk_renderer.hpp"

struct MeshResource {
  uint32_t startIndex;
  uint32_t indexCount;
  uint32_t startVertex;
  uint32_t vertexCount;
};

struct MeshResourceLoader {
  MeshResource operator()(const std::string &path,
                          VulkanRenderer &renderer) const;
};
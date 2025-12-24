#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "render/vk_renderer.hpp"

struct MeshResource {
  uint32_t renderer_id;
};

struct MeshResourceLoader {
  MeshResource operator()(const std::string &path,
                          VulkanRenderer &renderer) const;
};
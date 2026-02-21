#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

class Engine;

struct MeshResource
{
  uint32_t renderer_id;
  int32_t texture_id;
};

struct MeshResourceLoader
{
  MeshResource operator()(const std::string &path, Engine *engine) const;
};

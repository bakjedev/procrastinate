#pragma once

#include <cstdint>
#include <string>
#include <vector>


struct Vertex;
class Engine;

struct MeshResource
{
  uint32_t renderer_id;
  int32_t texture_id;
};

struct MeshObjResourceLoader
{
  MeshResource operator()(const std::string &path, Engine *engine) const;
};


struct MeshDataResourceLoader
{
  MeshResource operator()(std::vector<uint32_t> indices, std::vector<Vertex> vertices, Engine *engine) const;
};

#pragma once
#include "resource/resource.hpp"
#include "resource/types/mesh_resource.hpp"

struct CMesh
{
  ResourceRef<MeshResource> mesh;
  uint32_t mesh_id;
  int32_t texture_id;
};

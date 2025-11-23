#pragma once
#include "resource/resource.hpp"
#include "resource/types/mesh_resource.hpp"

struct MeshComponent {
  ResourceRef<MeshResource> mesh;
};
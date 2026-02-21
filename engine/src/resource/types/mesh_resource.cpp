#include "ecs/components/mesh_component.hpp"

#include <limits>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include "core/engine.hpp"
#include "files/files.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "util/print.hpp"

MeshResource MeshResourceLoader::operator()(const std::string &path, Engine *engine) const
{
  MeshResource res{};
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  tinyobj::attrib_t attrib;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  std::string err;
  std::string warn;

  std::string base_dir = path.substr(0, path.find_last_of("/\\") + 1);
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), base_dir.c_str()))
  {
    throw std::runtime_error(warn + err);
  }

  auto b_min = glm::vec3(std::numeric_limits<float>::max());
  auto b_max = -b_min;

  for (const auto &shape: shapes)
  {
    for (const auto &index: shape.mesh.indices)
    {
      const glm::vec3 pos = {attrib.vertices[index.vertex_index * 3], attrib.vertices[(index.vertex_index * 3) + 1],
                             attrib.vertices[(index.vertex_index * 3) + 2]};

      b_min = glm::min(pos, b_min);
      b_max = glm::max(pos, b_max);

      const glm::vec3 col = {attrib.colors[index.vertex_index * 3], attrib.colors[(index.vertex_index * 3) + 1],
                             attrib.colors[(index.vertex_index * 3) + 2]};

      auto nor = glm::vec3(0.0F);
      if (!attrib.normals.empty())
      {
        nor = {attrib.normals[index.normal_index * 3], attrib.normals[(index.normal_index * 3) + 1],
               attrib.normals[(index.normal_index * 3) + 2]};
      }
      glm::ivec2 tex_coord = glm::vec2(0.0F);
      if (!attrib.texcoords.empty())
      {
        tex_coord = {attrib.texcoords.at(index.texcoord_index * 2), attrib.texcoords.at(index.texcoord_index * 2) + 1};
      }

      vertices.emplace_back(pos, col, nor, tex_coord);
      indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
    }
  }

  const unsigned char *texture{};
  int32_t texture_width{};
  int32_t texture_height{};
  int32_t texture_channels{};
  for (const auto &material: materials)
  {
    if (!material.diffuse_texname.empty())
    {
      const auto tex_path = files::GetAssetsPathRoot().string() + "/engine/assets/" + material.diffuse_texname;
      if (!std::filesystem::exists(tex_path))
      {
        util::println("Failed to find texture");
        continue;
      }


      const auto *image =
          stbi_load(tex_path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);

      if (image == nullptr)
      {
        util::println("Failed to load texture");
        continue;
      }

      texture = image;
    }
  }

  auto &renderer = engine->GetRenderer();
  res.renderer_id =
      renderer.AddMesh(vertices, indices, renderer.GetIndexCount(), renderer.GetVertexCount(), b_min, b_max);
  if (texture != nullptr)
  {
    res.texture_id =
        renderer.AddTexture({texture, static_cast<size_t>(texture_width * texture_height * texture_channels)},
                            texture_width, texture_height);
  } else
  {
    res.texture_id = -1;
  }
  renderer.Upload();

  return res;
}

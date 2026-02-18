#pragma once

#include <filesystem>
#include <tuple>

#include "files/files.hpp"
#include "resource.hpp"
#include "resource/types/mesh_resource.hpp"
#include "types/shader_resource.hpp"

class ResourceManager
{
public:
  ResourceManager() { m_rootPath = files::GetResourceRoot(); }

  template<typename T>
  ResourceStorage<T> &getStorage()
  {
    return std::get<ResourceStorage<T>>(m_storages);
  }

  template<typename T>
  const ResourceStorage<T> &getStorage() const
  {
    return std::get<ResourceStorage<T>>(m_storages);
  }

  template<typename T, typename Loader, typename... Args>
  ResourceRef<T> create(const std::string &key, Loader &&loader, Args &&...args)
  {
    return getStorage<T>().create(key, std::forward<Loader>(loader), std::forward<Args>(args)...);
  }

  template<typename T, typename Loader, typename... Args>
  ResourceRef<T> createFromFile(const std::string &key, Loader &&loader, Args &&...args)
  {
    const std::string fullPath = (m_rootPath / key).string();
    return getStorage<T>().create(key, std::forward<Loader>(loader), fullPath, std::forward<Args>(args)...);
  }

private:
  std::tuple<ResourceStorage<ShaderResource>, ResourceStorage<MeshResource>> m_storages;

  std::filesystem::path m_rootPath;
};

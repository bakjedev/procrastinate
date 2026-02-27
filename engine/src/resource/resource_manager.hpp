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
  ResourceManager() { root_path_ = files::GetAssetsPathRoot(); }

  template<typename T>
  ResourceStorage<T> &GetStorage()
  {
    return std::get<ResourceStorage<T>>(storages_);
  }

  template<typename T>
  const ResourceStorage<T> &GetStorage() const
  {
    return std::get<ResourceStorage<T>>(storages_);
  }

  template<typename T, typename Loader, typename... Args>
  ResourceHandle<T> load(const std::string &key, Loader &&loader, Args &&...args)
  {
    return GetStorage<T>().load(key, std::forward<Loader>(loader), std::forward<Args>(args)...);
  }

  template<typename T, typename Loader, typename... Args>
  ResourceHandle<T> CreateFromFile(const std::string &key, Loader &&loader, Args &&...args)
  {
    const std::string full_path = (root_path_ / key).string();
    return GetStorage<T>().load(key, std::forward<Loader>(loader), full_path, std::forward<Args>(args)...);
  }

private:
  std::tuple<ResourceStorage<ShaderResource>, ResourceStorage<MeshResource>> storages_;

  std::filesystem::path root_path_;
};

#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
class ResourcePool {
 private:
  struct Handle {
    uint32_t index;
    uint32_t generation;
  };

  struct Metadata {
    uint32_t generation;
    uint32_t refCount;
    std::string key;
  };
  std::vector<T> m_resources;
  std::vector<Metadata> m_metadata;
  std::vector<uint32_t> m_freeList;
  std::unordered_map<std::string, uint32_t> m_keyToIndex;

 public:
  std::optional<Handle> find(const std::string& key) const {
    auto it = m_keyToIndex.find(key);
    if (it != m_keyToIndex.end()) {
      uint32_t index = it->second;
      return {index, m_metadata[index].generation};
    }
    return std::nullopt;
  }

  Handle allocate() {
    uint32_t index;
    Metadata* metadata;

    if (!m_freeList.empty()) {
      index = m_freeList.back();
      m_freeList.pop_back();

      metadata = &m_metadata[index];
      metadata->generation++;
    } else {
      index = m_resources.size();
      m_resources.emplace_back();

      metadata = &m_metadata.emplace_back();
      metadata->generation = 1;
    }

    metadata->refCount = 1;

    return {index, metadata->generation};
  }

  void registerKey(const std::string& key, uint32_t index) {
    m_keyToIndex[key] = index;
    m_metadata[index].key = key;
  }

  void release(Handle handle) {
    if (!isValid(handle)) {
      return;
    }

    auto& metadata = m_metadata[handle.index];

    if (metadata.refCount == 0) {
      return;
    }

    metadata.refCount--;

    if (metadata.refCount == 0) {
      m_freeList.push_back(handle.index);
      metadata.generation = 0;
      if (!metadata.key.empty()) {
        m_keyToIndex.erase(metadata.key);
      }
    }
  }

  void addRef(Handle handle) {
    if (isValid(handle)) {
      m_metadata[handle.index].refCount++;
    }
  }

  template <typename Loader, typename... Args>
  bool load(Handle handle, Loader&& loader, Args&&... args) {
    if (!isValid(handle)) return false;

    m_resources[handle.index] = loader(std::forward<Args>(args)...);

    return true;
  }

  bool isValid(Handle handle) {
    if (handle.index >= m_metadata.size()) {
      return false;
    }

    const auto& metadata = m_metadata[handle.index];

    return handle.generation != 0 && metadata.generation == handle.generation;
  }
};
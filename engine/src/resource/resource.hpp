#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
class ResourceHandle;

template <typename T>
struct HandleHash {
  std::size_t operator()(const ResourceHandle<T>& handle) const {
    return std::hash<uint64_t>{}((static_cast<uint64_t>(handle.m_index) << 32) |
                                 handle.m_generation);
  }
};

template <typename T>
class ResourceStorage {
 private:
  using Handle = ResourceHandle<T>;

  struct Metadata {
    uint32_t generation;
    uint32_t refCount;

    Metadata(uint32_t g, uint32_t r) : generation(g), refCount(r) {}
  };

  std::vector<T> m_resources;
  std::vector<Metadata> m_metadata;
  std::vector<uint32_t> m_free;

  std::unordered_map<std::string, Handle> m_keyToHandle;
  std::unordered_map<Handle, std::string, HandleHash<T>> m_handleToKey;

 public:
  template <typename Loader, typename... Args>
  Handle create(const std::string& key, Loader&& loader, Args&&... args) {
    auto it = m_keyToHandle.find(key);
    if (it != m_keyToHandle.end()) {
      const Handle& existingHandle = it->second;
      if (is_valid(existingHandle)) {
        acquire(existingHandle);
        return existingHandle;
      } else {
        m_keyToHandle.erase(it);
        m_handleToKey.erase(existingHandle);
      }
    }

    uint32_t index;

    if (!m_free.empty()) {
      index = m_free.back();
      m_free.pop_back();

      m_metadata[index].refCount = 1;

      m_resources[index] = loader(std::forward<Args>(args)...);
    } else {
      index = m_resources.size();

      m_metadata.emplace_back(1, 1);

      m_resources.push_back(loader(std::forward<Args>(args)...));
    }

    Handle handle{index, m_metadata[index].generation};
    m_keyToHandle[key] = handle;
    m_handleToKey[handle] = key;
    return handle;
  }

  void destroy(const Handle& handle) {
    uint32_t index = handle.m_index;
    if (index >= m_metadata.size()) return;

    auto& metadata = m_metadata[handle.m_index];

    if (metadata.generation != handle.m_generation) return;

    if (--metadata.refCount == 0) {
      auto it = m_handleToKey.find(handle);
      if (it != m_handleToKey.end()) {
        m_keyToHandle.erase(it->second);
        m_handleToKey.erase(it);
      }

      m_free.push_back(handle.m_index);

      ++metadata.generation;
      // could destroy resource here but why not leave it, gets destroyed when
      // replaced anyway.
    }
  }

  void acquire(const Handle& handle) {
    uint32_t index = handle.m_index;
    if (index < m_metadata.size() &&
        m_metadata[index].generation == handle.m_generation)
      ++m_metadata[index].refCount;
  }

  T* get(const Handle& handle) {
    uint32_t index = handle.m_index;
    if (index >= m_resources.size()) return nullptr;
    if (m_metadata[index].generation != handle.m_generation) return nullptr;
    return &m_resources[index];
  }

  const T* get(const Handle& handle) const {
    uint32_t index = handle.m_index;
    if (index >= m_resources.size()) return nullptr;
    if (m_metadata[index].generation != handle.m_generation) return nullptr;
    return &m_resources[index];
  }

  bool is_valid(const Handle& handle) const {
    uint32_t index = handle.m_index;
    return index < m_metadata.size() &&
           m_metadata[index].generation == handle.m_generation;
  }
};

template <typename T>
class ResourceHandle {
 public:
  ResourceHandle() = default;

  bool operator==(const ResourceHandle& other) const {
    return m_index == other.m_index && m_generation == other.m_generation;
  }

  ResourceHandle(uint32_t index, uint32_t generation)
      : m_index(index), m_generation(generation) {}

 private:
  friend class ResourceStorage<T>;
  friend struct HandleHash<T>;
  uint32_t m_index;
  uint32_t m_generation;
};
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
struct ResourceHandle {
  uint32_t index = 0;
  uint32_t generation = 0;

  bool operator==(const ResourceHandle&) const = default;
};

template <typename T>
struct ResourceHandleHash {
  std::size_t operator()(const ResourceHandle<T>& handle) const {
    return std::hash<uint64_t>{}((static_cast<uint64_t>(handle.index) << 32) |
                                 handle.generation);
  }
};

template <typename T>
class ResourceStorage;

template <typename T>
class ResourceRef {
 public:
  ResourceRef() = default;

  ResourceRef(const ResourceRef& other)
      : m_handle(other.m_handle), m_storage(other.m_storage) {
    if (m_storage) {
      m_storage->acquire(m_handle);
    }
  }

  ResourceRef(ResourceRef&& other) noexcept
      : m_handle(other.m_handle), m_storage(other.m_storage) {
    other.m_storage = nullptr;
  }

  ResourceRef& operator=(const ResourceRef& other) {
    if (this != &other) {
      if (m_storage) {
        m_storage->destroy(m_handle);
      }

      m_handle = other.m_handle;
      m_storage = other.m_storage;

      if (m_storage) {
        m_storage->acquire(m_handle);
      }
    }
    return *this;
  }

  ResourceRef& operator=(ResourceRef&& other) noexcept {
    if (this != &other) {
      if (m_storage) {
        m_storage->destroy(m_handle);
      }

      m_handle = other.m_handle;
      m_storage = other.m_storage;

      other.m_storage = nullptr;
    }
    return *this;
  }

  ~ResourceRef() {
    if (m_storage) {
      m_storage->destroy(m_handle);
    }
  }

  // make it act like a pointer to the resource
  T* get() const { return m_storage ? m_storage->get(m_handle) : nullptr; }

  T* operator->() const { return get(); }

  T& operator*() const {
    auto* ptr = get();
    assert(ptr && "dereferencing nullptr");
    return *ptr;
  }

  bool isValid() const { return m_storage && m_storage->isValid(m_handle); }

  ResourceHandle<T> handle() const { return m_handle; }

 private:
  friend class ResourceStorage<T>;

  ResourceRef(ResourceHandle<T> handle, ResourceStorage<T>* storage)
      : m_handle(handle), m_storage(storage) {}

  ResourceHandle<T> m_handle;
  ResourceStorage<T>* m_storage;
};

template <typename T>
class ResourceStorage {
 private:
  using Ref = ResourceRef<T>;
  using Handle = ResourceHandle<T>;

  struct Metadata {
    uint32_t generation;
    uint32_t refCount;

    Metadata(uint32_t g, uint32_t r) : generation(g), refCount(r) {}
  };

  std::vector<T> m_resources;
  std::vector<Metadata> m_metadata;
  std::vector<uint32_t> m_free;

  std::unordered_map<std::string, ResourceHandle<T>> m_keyToHandle;
  std::unordered_map<ResourceHandle<T>, std::string, ResourceHandleHash<T>>
      m_handleToKey;

 public:
  template <typename Loader, typename... Args>
  Ref create(const std::string& key, Loader&& loader, Args&&... args) {
    auto it = m_keyToHandle.find(key);
    if (it != m_keyToHandle.end()) {
      const Handle& cached = it->second;
      if (isValid(cached)) {
        ++m_metadata[cached.index].refCount;
        return Ref{cached, this};
      } else {
        m_keyToHandle.erase(it);
        m_handleToKey.erase(cached);
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
    m_keyToHandle.insert({key, handle});
    m_handleToKey.insert({handle, key});
    return Ref{handle, this};
  }

  void destroy(const Handle& handle) {
    uint32_t index = handle.index;
    if (index >= m_metadata.size()) return;

    auto& metadata = m_metadata[handle.index];

    if (metadata.generation != handle.generation) return;

    if (--metadata.refCount == 0) {
      auto it = m_handleToKey.find(handle);
      if (it != m_handleToKey.end()) {
        m_keyToHandle.erase(it->second);
        m_handleToKey.erase(it);
      }

      m_free.push_back(handle.index);

      ++metadata.generation;
      // could destroy resource here but why not leave it, gets destroyed when
      // replaced anyway.
    }
  }

  void acquire(const Handle& handle) {
    if (isValid(handle)) ++m_metadata[handle.index].refCount;
  }

  T* get(const Handle& handle) {
    if (isValid(handle)) {
      return &m_resources[handle.index];
    }
    return nullptr;
  }

  const T* get(const Handle& handle) const {
    if (isValid(handle)) {
      return &m_resources[handle.index];
    }
  }

  bool isValid(const Handle& handle) const {
    uint32_t index = handle.index;
    return index < m_metadata.size() &&
           m_metadata[index].generation == handle.generation;
  }
};
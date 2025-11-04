#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
class ResourceHandle;

template <typename T>
class ResourcePool {
 private:
  struct Metadata {
    uint32_t generation;
    uint32_t refCount;
    std::string key;
  };
  std::vector<std::optional<T>> m_resources;
  std::vector<Metadata> m_metadata;
  std::vector<uint32_t> m_freeList;
  std::unordered_map<std::string, uint32_t> m_keyToIndex;

  std::optional<ResourceHandle<T>> find(const std::string& key) {
    auto it = m_keyToIndex.find(key);
    if (it != m_keyToIndex.end()) {
      uint32_t index = it->second;
      m_metadata[index].refCount++;
      return ResourceHandle<T>(this, index, m_metadata[index].generation);
    }
    return std::nullopt;
  }

  ResourceHandle<T> allocate() {
    uint32_t index;
    Metadata* metadata;

    if (!m_freeList.empty()) {
      index = m_freeList.back();
      m_freeList.pop_back();

      metadata = &m_metadata[index];
      metadata->generation++;
      // should probably do something when we have 4 billion plus generations
      // (if that ever happens)
    } else {
      index = m_resources.size();
      m_resources.emplace_back(std::nullopt);

      m_metadata.emplace_back();
      metadata = &m_metadata.back();
      metadata->generation = 1;
    }

    metadata->refCount = 1;

    return ResourceHandle<T>(this, index, metadata->generation);
  }

  void registerKey(const std::string& key, uint32_t index) {
    m_keyToIndex[key] = index;
    m_metadata[index].key = key;
  }

  template <typename Loader, typename... Args>
  bool loadIntoEntry(const ResourceHandle<T>& handle, Loader&& loader,
                     Args&&... args) {
    if (!isValid(handle)) return false;
    m_resources[handle.index].emplace(loader(std::forward<Args>(args)...));

    return true;
  }

  bool isValid(const ResourceHandle<T>& handle) {
    if (handle.index >= m_metadata.size()) {
      return false;
    }

    const auto& metadata = m_metadata[handle.index];

    return handle.generation != 0 && metadata.generation == handle.generation;
  }

  T* get(const ResourceHandle<T>& handle) {
    if (isValid(handle) && m_resources[handle.index].has_value()) {
      return &m_resources[handle.index].value();
    }
    return nullptr;
  }

  void release(const ResourceHandle<T>& handle) {
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
      if (!metadata.key.empty()) {
        m_keyToIndex.erase(metadata.key);
      }
      // currently don't deconstruct resource cuz might be heavy. maybe later i
      // periodically remove them. get destroyed anyway when replaced
    }
  }

  void addRef(const ResourceHandle<T>& handle) {
    if (isValid(handle)) {
      m_metadata[handle.index].refCount++;
    }
  }

  friend class ResourceHandle<T>;

 public:
  template <typename Loader, typename... Args>
  ResourceHandle<T> load(const std::string& key, Loader&& loader,
                         Args&&... args) {
    if (auto cachedHandle = find(key)) {
      return *cachedHandle;
    }

    auto handle = allocate();

    registerKey(key, handle.index);

    if (!loadIntoEntry(handle, std::forward<Loader>(loader),
                       std::forward<Args>(args)...)) {
      release(handle);
      return ResourceHandle<T>();
    }

    return handle;
  }

  template <typename Loader, typename... Args>
  ResourceHandle<T> loadFile(const std::string& key, Loader&& loader) {
    return load(key, std::forward<Loader>(loader), key);
  }
};

template <typename T>
class ResourceHandle {
 private:
  ResourcePool<T>* m_pool = nullptr;

  ResourceHandle(ResourcePool<T>* pool, uint32_t idx, uint32_t gen)
      : m_pool(pool), index(idx), generation(gen) {}
  friend class ResourcePool<T>;

  void addRef() {
    if (m_pool) {
      m_pool->addRef(*this);
    }
  }

  void release() {
    if (m_pool) {
      m_pool->release(*this);
      m_pool = nullptr;
    }
  }

 public:
  uint32_t index = 0;
  uint32_t generation = 0;

  ResourceHandle() = default;

  ResourceHandle(const ResourceHandle& other)
      : m_pool(other.m_pool), index(other.index), generation(other.generation) {
    addRef();
  }

  ResourceHandle& operator=(const ResourceHandle& other) {
    if (this != &other) {
      release();
      m_pool = other.m_pool;
      index = other.index;
      generation = other.generation;
      addRef();
    }
    return *this;
  }

  ResourceHandle(ResourceHandle&& other) noexcept
      : m_pool(other.m_pool), index(other.index), generation(other.generation) {
    other.m_pool = nullptr;
    other.index = 0;
    other.generation = 0;
  }

  ResourceHandle& operator=(ResourceHandle&& other) noexcept {
    if (this != &other) {
      release();
      m_pool = other.m_pool;
      index = other.index;
      generation = other.generation;
      other.m_pool = nullptr;
      other.index = 0;
      other.generation = 0;
    }
    return *this;
  }

  ~ResourceHandle() { release(); }

  T* operator->() const { return m_pool ? m_pool->get(*this) : nullptr; }
  T& operator*() const {
    T* ptr = operator->();
    assert(ptr && "dereferencing invalid ResourceHandle");
    return *ptr;
  }
  T* get() const { return operator->(); }

  explicit operator bool() const { return get() != nullptr; }

  bool operator==(const ResourceHandle& other) const {
    return m_pool == other.m_pool && index == other.index &&
           generation == other.generation;
  }

  bool operator!=(const ResourceHandle& other) const {
    return !(*this == other);
  }
};
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename T>
struct ResourceHandle
{
  uint32_t index = 0;
  uint32_t generation = 0;

  bool operator==(const ResourceHandle&) const = default;
};

template<typename T>
struct ResourceHandleHash
{
  std::size_t operator()(const ResourceHandle<T>& handle) const
  {
    constexpr std::size_t generation_bits = 32;

    return std::hash<uint64_t>{}((static_cast<uint64_t>(handle.index) << generation_bits) | handle.generation);
  }
};

template<typename T>
class ResourceStorage;

template<typename T>
class ResourceRef
{
public:
  ResourceRef() = default;

  ResourceRef(const ResourceRef& other) : handle_(other.handle_), storage_(other.storage_)
  {
    if (storage_)
    {
      storage_->acquire(handle_);
    }
  }

  ResourceRef(ResourceRef&& other) noexcept : handle_(other.handle_), storage_(other.storage_)
  {
    other.storage_ = nullptr;
  }

  ResourceRef& operator=(const ResourceRef& other)
  {
    if (this != &other)
    {
      if (storage_)
      {
        storage_->destroy(handle_);
      }

      handle_ = other.handle_;
      storage_ = other.storage_;

      if (storage_)
      {
        storage_->acquire(handle_);
      }
    }
    return *this;
  }

  ResourceRef& operator=(ResourceRef&& other) noexcept
  {
    if (this != &other)
    {
      if (storage_)
      {
        storage_->destroy(handle_);
      }

      handle_ = other.handle_;
      storage_ = other.storage_;

      other.storage_ = nullptr;
    }
    return *this;
  }

  ~ResourceRef()
  {
    if (storage_)
    {
      storage_->destroy(handle_);
    }
  }

  // make it act like a pointer to the resource
  T* get() { return storage_ ? storage_->get(handle_) : nullptr; }
  T* operator->() { return get(); }
  T& operator*()
  {
    auto* ptr = get();
    assert(ptr && "dereferencing nullptr");
    return *ptr;
  }

  const T* get() const { return storage_ ? storage_->get(handle_) : nullptr; }
  const T* operator->() const { return get(); }
  const T& operator*() const
  {
    auto* ptr = get();
    assert(ptr && "dereferencing nullptr");
    return *ptr;
  }

  [[nodiscard]] bool valid() const { return storage_ && storage_->valid(handle_); }

private:
  friend class ResourceStorage<T>;

  ResourceRef(ResourceHandle<T> handle, ResourceStorage<T>* storage) : handle_(handle), storage_(storage) {}

  ResourceHandle<T> handle_;
  ResourceStorage<T>* storage_;
};

// concept for valid loaders. needs () operator that takes the right args and
// returns the right type
template<typename Loader, typename T, typename... Args>
concept LoaderFor = std::invocable<Loader, Args...> && std::same_as<std::invoke_result_t<Loader, Args...>, T>;

template<typename T>
class ResourceStorage
{
private:
  using Ref = ResourceRef<T>;
  using Handle = ResourceHandle<T>;

  friend class ResourceRef<T>;

  struct Metadata
  {
    uint32_t generation;
    uint32_t ref_count;
  };

  std::vector<T> resources_;
  std::vector<Metadata> metadata_;
  std::vector<uint32_t> free_;

  std::unordered_map<std::string, ResourceHandle<T>> key_to_handle_;
  std::unordered_map<ResourceHandle<T>, std::string, ResourceHandleHash<T>> handle_to_key_;

  void destroy(const Handle& handle)
  {
    uint32_t index = handle.index;
    if (index >= metadata_.size())
    {
      return;
    }

    auto& metadata = metadata_[handle.index];

    if (metadata.generation != handle.generation)
    {
      return;
    }

    if (--metadata.ref_count == 0)
    {
      auto iter = handle_to_key_.find(handle);
      if (iter != handle_to_key_.end())
      {
        key_to_handle_.erase(iter->second);
        handle_to_key_.erase(iter);
      }

      free_.push_back(handle.index);

      ++metadata.generation;
      // could destroy resource here but why not leave it, gets destroyed when
      // replaced anyway.
    }
  }

  void acquire(const Handle& handle)
  {
    if (valid(handle))
    {
      ++metadata_[handle.index].ref_count;
    }
  }

  const T* get(const Handle& handle) const
  {
    if (valid(handle))
    {
      return &resources_[handle.index];
    }
    return nullptr;
  }

  T* get(const Handle& handle)
  {
    if (valid(handle))
    {
      return &resources_[handle.index];
    }
    return nullptr;
  }

public:
  template<typename Loader, typename... Args>
    requires LoaderFor<Loader, T, Args...>
  Ref create(const std::string& key, Loader&& loader, Args&&... args)
  {
    auto iter = key_to_handle_.find(key);
    if (iter != key_to_handle_.end())
    {
      const Handle& cached = iter->second;
      if (valid(cached))
      {
        ++metadata_[cached.index].ref_count;
        return Ref{cached, this};
      } else
      {
        key_to_handle_.erase(iter);
        handle_to_key_.erase(cached);
      }
    }

    // load before touching metadata because might throw exception
    T resource = std::forward<Loader>(loader)(std::forward<Args>(args)...);

    uint32_t index = 0;

    if (!free_.empty())
    {
      index = free_.back();
      free_.pop_back();

      metadata_[index].ref_count = 1;

      resources_[index] = std::move(resource);
    } else
    {
      index = resources_.size();

      metadata_.push_back(Metadata{.generation = 1, .ref_count = 1});

      resources_.push_back(std::move(resource));
    }

    Handle handle{index, metadata_[index].generation};
    key_to_handle_[key] = handle;
    handle_to_key_[handle] = key;
    return Ref{handle, this};
  }

  Ref get(const std::string& key)
  {
    auto iter = key_to_handle_.find(key);
    if (iter != key_to_handle_.end())
    {
      const Handle& cached = iter->second;
      if (valid(cached))
      {
        ++metadata_[cached.index].refCount;
        return Ref{cached, this};
      } else
      {
        key_to_handle_.erase(iter);
        handle_to_key_.erase(cached);
      }
    }
    return Ref{};
  }

  bool valid(const Handle& handle) const
  {
    uint32_t index = handle.index;
    return index < metadata_.size() && metadata_[index].generation == handle.generation;
  }

  bool valid(const Handle& handle)
  {
    uint32_t index = handle.index;
    return index < metadata_.size() && metadata_[index].generation == handle.generation;
  }
};

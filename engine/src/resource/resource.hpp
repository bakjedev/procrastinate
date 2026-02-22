#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename T>
class ResourceStorage;

template<typename T>
class ResourceRef
{
public:
  ResourceRef() = default;
  ~ResourceRef() { release(); }

  ResourceRef(const ResourceRef& other) : index_(other.index_), storage_(other.storage_)
  {
    if (valid())
    {
      storage_->acquire(index_);
    }
  }

  ResourceRef(ResourceRef&& other) noexcept : index_(other.index_), storage_(other.storage_)
  {
    other.storage_ = nullptr;
  }

  ResourceRef& operator=(const ResourceRef& other)
  {
    if (this != &other)
    {
      release();
      index_ = other.index_;
      storage_ = other.storage_;
      if (valid())
      {
        storage_->acquire(index_);
      }
    }
    return *this;
  }

  ResourceRef& operator=(ResourceRef&& other) noexcept
  {
    if (this != &other)
    {
      release();
      index_ = other.index_;
      storage_ = other.storage_;
      other.storage_ = nullptr;
    }
    return *this;
  }

  T* operator->() { return &storage_->resources_[index_]; }
  T& operator*() { return storage_->resources_[index_]; }
  const T* operator->() const { return &storage_->resources_[index_]; }
  const T& operator*() const { return storage_->resources_[index_]; }

  [[nodiscard]] bool valid() const { return storage_ != nullptr; }
  explicit operator bool() const { return valid(); }

private:
  friend class ResourceStorage<T>;

  ResourceRef(const uint32_t index, ResourceStorage<T>* storage) : index_(index), storage_(storage) {}

  void release()
  {
    if (valid())
    {
      storage_->release(index_);
    }
    storage_ = nullptr;
  }

  uint32_t index_ = 0;
  ResourceStorage<T>* storage_;
};

// concept for valid loaders. needs () operator that takes the right args and
// returns the right type
template<typename Loader, typename T, typename... Args>
concept LoaderFor = std::invocable<Loader, Args...> && std::same_as<std::invoke_result_t<Loader, Args...>, T>;

template<typename T>
class ResourceStorage
{
  using Ref = ResourceRef<T>;

  friend class ResourceRef<T>;

  std::vector<T> resources_;
  std::vector<uint32_t> ref_counts_;
  std::vector<std::string> keys_;
  std::vector<uint32_t> free_;

  std::unordered_map<std::string, uint32_t> key_to_index_;

  void acquire(const uint32_t index) { ++ref_counts_.at(index); }

  void release(const uint32_t index)
  {
    assert(ref_counts_.at(index) > 0);
    if (--ref_counts_.at(index) == 0)
    {
      key_to_index_.erase(keys_.at(index));
      keys_.at(index).clear();
      // resources_[index] = T{};
      // could destroy resource here but why not leave it, gets destroyed when
      // replaced anyway.
      free_.push_back(index);
    }
  }

public:
  template<typename Loader, typename... Args>
    requires LoaderFor<Loader, T, Args...>
  ResourceRef<T> load(const std::string& key, Loader&& loader, Args&&... args)
  {
    auto iter = key_to_index_.find(key);
    if (iter != key_to_index_.end())
    {
      ++ref_counts_.at(iter->second);
      return ResourceRef<T>{iter->second, this};
    }

    // load before touching indices because might throw exception
    T resource = std::forward<Loader>(loader)(std::forward<Args>(args)...);

    uint32_t index{};
    if (!free_.empty())
    {
      index = free_.back();
      free_.pop_back();
      resources_[index] = std::move(resource);
    } else
    {
      index = static_cast<uint32_t>(resources_.size());
      resources_.push_back(std::move(resource));
      ref_counts_.emplace_back();
      keys_.emplace_back();
    }

    ref_counts_.at(index) = 1;
    keys_.at(index) = key;
    key_to_index_[key] = index;

    return ResourceRef<T>{index, this};
  }

  ResourceRef<T> get(const std::string& key)
  {
    auto iter = key_to_index_.find(key);
    if (iter == key_to_index_.end())
    {
      return {};
    }
    ++ref_counts_.at(iter->second);
    return ResourceRef<T>{iter->second, this};
  }
};

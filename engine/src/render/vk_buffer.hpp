#pragma once
#include <vma/vma_usage.h>

#include <vulkan/vulkan.hpp>

struct BufferInfo
{
  size_t size{};
  vk::BufferUsageFlags usage;
  VmaMemoryUsage memoryUsage{};
  VmaAllocationCreateFlags memoryFlags{};
};

class VulkanDevice;

class VulkanBuffer
{
public:
  explicit VulkanBuffer(const BufferInfo& info, VmaAllocator allocator, VulkanDevice* device);
  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer(VulkanBuffer&&) = delete;
  VulkanBuffer& operator=(const VulkanBuffer&) = delete;
  VulkanBuffer& operator=(VulkanBuffer&&) = delete;
  ~VulkanBuffer();

  void Create(const BufferInfo& info);
  void Write(const void* src_data);
  void WriteRange(const void* src_data, uint32_t range_size);
  void WriteRangeOffset(const void* src_data, uint32_t range_size, uint32_t offset);

  void* map();
  void unmap() const;

  [[nodiscard]] void* GetMappedData() const { return mapped_data_; };
  template<typename T>
  [[nodiscard]] T* GetMappedDataAs() const
  {
    return static_cast<T*>(mapped_data_);
  };

  void Destroy();

  [[nodiscard]] vk::Buffer get() const { return buffer_; }
  [[nodiscard]] VmaAllocation allocation() const { return allocation_; }
  [[nodiscard]] uint32_t size() const { return size_; }

private:
  vk::Buffer buffer_;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  uint32_t size_ = 0;
  void* mapped_data_ = nullptr;
  VmaAllocator allocator_;
  VulkanDevice* device_;
};

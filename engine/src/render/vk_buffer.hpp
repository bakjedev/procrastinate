#pragma once
#include <vma/vma_usage.h>

#include <vulkan/vulkan.hpp>

struct BufferInfo {
  size_t size{};
  vk::BufferUsageFlags usage;
  VmaMemoryUsage memoryUsage{};
  VmaAllocationCreateFlags memoryFlags{};
};

class VulkanBuffer {
 public:
  explicit VulkanBuffer(const BufferInfo& info, VmaAllocator allocator);
  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer(VulkanBuffer&&) = delete;
  VulkanBuffer& operator=(const VulkanBuffer&) = delete;
  VulkanBuffer& operator=(VulkanBuffer&&) = delete;
  ~VulkanBuffer();

  void create(const BufferInfo& info);
  void write(const void* srcData);
  void writeRange(const void* srcData, uint32_t rangeSize);
  void writeRangeOffset(const void* srcData, uint32_t rangeSize,
                        uint32_t offset);

  void* map();
  void unmap();

  [[nodiscard]] void* getMappedData() const { return m_mappedData; };

  void destroy();

  [[nodiscard]] vk::Buffer get() const { return m_buffer; }
  [[nodiscard]] VmaAllocation allocation() const { return m_allocation; }
  [[nodiscard]] uint32_t size() const { return m_size; }

 private:
  vk::Buffer m_buffer;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
  uint32_t m_size = 0;
  void* m_mappedData = nullptr;
  VmaAllocator m_allocator;
};
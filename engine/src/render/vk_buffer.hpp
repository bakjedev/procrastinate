#pragma once
#include <vma/vma_usage.h>

struct BufferInfo {
  uint32_t size{};
  VkBufferUsageFlags usage{};
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
  void map(const void* srcData);
  void mapRange(const void* srcData, uint32_t rangeSize);
  void destroy();

  [[nodiscard]] VkBuffer get() const { return m_buffer; }
  [[nodiscard]] VmaAllocation allocation() const { return m_allocation; }
  [[nodiscard]] uint32_t size() const { return m_size; }

 private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
  uint32_t m_size = 0;
  VmaAllocator m_allocator;
};
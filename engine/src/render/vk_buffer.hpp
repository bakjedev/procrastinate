#pragma once
#include <vma/vma_usage.h>

class VulkanBuffer {
 public:
  VulkanBuffer(VmaAllocator allocator, uint32_t bufferSize,
               VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage,
               VmaAllocationCreateFlags memoryFlags = 0U);
  ~VulkanBuffer();
  void create(uint32_t bufferSize, VkBufferUsageFlags bufferUsage,
              VmaMemoryUsage memoryUsage,
              VmaAllocationCreateFlags memoryFlags = 0U);
  void map(const void* srcData);
  void mapRange(const void* srcData, uint32_t rangeSize);
  void destroy();

  VkBuffer get() const { return m_buffer; }
  VmaAllocation allocation() const { return m_allocation; }
  uint32_t size() const { return m_size; }

 private:
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
  uint32_t m_size = 0;
  VmaAllocator m_allocator;
};
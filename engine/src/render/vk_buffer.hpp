#pragma once
#include <vma/vma_usage.h>

class VulkanBuffer {
 public:
  VulkanBuffer(VmaAllocator allocator, unsigned int bufferSize,
               VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage,
               VmaAllocationCreateFlags memoryFlags = 0U);
  ~VulkanBuffer();
  void create(unsigned int bufferSize, VkBufferUsageFlags bufferUsage,
              VmaMemoryUsage memoryUsage,
              VmaAllocationCreateFlags memoryFlags = 0U);
  void map(void* srcData);
  void mapRange(void* srcData, unsigned int rangeSize);
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
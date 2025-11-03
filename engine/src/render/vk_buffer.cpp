#include "vk_buffer.hpp"

#include <cstring>

#include "util/util.hpp"


VulkanBuffer::VulkanBuffer(VmaAllocator allocator, unsigned int bufferSize,
                           VkBufferUsageFlags bufferUsage,
                           VmaMemoryUsage memoryUsage,
                           VmaAllocationCreateFlags memoryFlags)
    : m_allocator(allocator) {
  create(bufferSize, bufferUsage, memoryUsage, memoryFlags);
}

VulkanBuffer::~VulkanBuffer() { destroy(); }

void VulkanBuffer::create(unsigned int bufferSize,
                          VkBufferUsageFlags bufferUsage,
                          VmaMemoryUsage memoryUsage,
                          VmaAllocationCreateFlags memoryFlags) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = bufferUsage;
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;
  vmaallocInfo.flags = memoryFlags;
  VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &vmaallocInfo, &m_buffer,
                           &m_allocation, nullptr));
  m_size = bufferSize;
}

void VulkanBuffer::map(void* srcData) {
  void* data;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, m_size);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::mapRange(void* srcData, unsigned int rangeSize) {
  void* data;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, rangeSize);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::destroy() {
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
  m_buffer = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
}

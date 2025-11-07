#include "vk_buffer.hpp"

#include <cstring>

#include "util/vk_check.hpp"

VulkanBuffer::VulkanBuffer(const BufferInfo& info, VmaAllocator allocator)
    : m_allocator(allocator) {
  create(info);
}

VulkanBuffer::~VulkanBuffer() { destroy(); }

void VulkanBuffer::create(const BufferInfo& info) {
  VkBufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.pNext = nullptr;
  bufferCreateInfo.size = info.size;
  bufferCreateInfo.usage = info.usage;
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = info.memoryUsage;
  vmaallocInfo.flags = info.memoryFlags;
  VK_CHECK(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &vmaallocInfo,
                           &m_buffer, &m_allocation, nullptr));
  m_size = info.size;
}

void VulkanBuffer::map(const void* srcData) {
  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, m_size);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::mapRange(const void* srcData, uint32_t rangeSize) {
  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, rangeSize);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::destroy() {
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
  m_buffer = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
}

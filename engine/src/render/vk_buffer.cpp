#include "vk_buffer.hpp"

#include <cstring>

#include "util/vk_check.hpp"

VulkanBuffer::VulkanBuffer(const BufferInfo& info, VmaAllocator allocator)
    : m_allocator(allocator) {
  create(info);
}

VulkanBuffer::~VulkanBuffer() { destroy(); }

void VulkanBuffer::create(const BufferInfo& info) {
  vk::BufferCreateInfo bufferCreateInfo{.size = info.size, .usage = info.usage};

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = info.memoryUsage;
  vmaallocInfo.flags = info.memoryFlags;

  auto rawBufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

  VkBuffer tempBuffer = VK_NULL_HANDLE;
  VK_CHECK(vmaCreateBuffer(m_allocator, &rawBufferInfo, &vmaallocInfo,
                           &tempBuffer, &m_allocation, nullptr));
  m_buffer = tempBuffer;
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
  m_buffer = nullptr;
  m_allocation = VK_NULL_HANDLE;
}

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

  VmaAllocationInfo allocationInfo;

  VkBuffer tempBuffer = VK_NULL_HANDLE;
  VK_CHECK(vmaCreateBuffer(m_allocator, &rawBufferInfo, &vmaallocInfo,
                           &tempBuffer, &m_allocation, &allocationInfo));
  m_buffer = tempBuffer;
  m_size = info.size;
  m_mappedData = allocationInfo.pMappedData;
}

void VulkanBuffer::write(const void* srcData) {
  if (m_mappedData != nullptr) {
    memcpy(m_mappedData, srcData, m_size);
    return;
  }

  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, m_size);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::writeRange(const void* srcData, uint32_t rangeSize) {
  if (m_mappedData != nullptr) {
    memcpy(m_mappedData, srcData, rangeSize);
    return;
  }

  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(data, srcData, rangeSize);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void VulkanBuffer::writeRangeOffset(const void* srcData, uint32_t rangeSize,
                                    uint32_t offset) {
  if (m_mappedData != nullptr) {
    memcpy(static_cast<uint8_t*>(m_mappedData) + offset, srcData, rangeSize);
    return;
  }

  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);
  memcpy(static_cast<uint8_t*>(data) + offset, srcData, rangeSize);
  vmaUnmapMemory(m_allocator, m_allocation);
}

void* VulkanBuffer::map(void* data) {
  if (m_mappedData != nullptr) {
    vmaMapMemory(m_allocator, m_allocation, &m_mappedData);
    return m_mappedData;
  }

  if (data != nullptr) {
    vmaMapMemory(m_allocator, m_allocation, &data);
    return data;
  }
  return nullptr;
}

void VulkanBuffer::unmap() { vmaUnmapMemory(m_allocator, m_allocation); }

void VulkanBuffer::destroy() {
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
  m_buffer = nullptr;
  m_allocation = VK_NULL_HANDLE;
  m_mappedData = nullptr;
}

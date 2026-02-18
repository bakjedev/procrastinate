#include "vk_buffer.hpp"

#include <cstring>
#include <unordered_set>

#include "util/vk_check.hpp"
#include "vk_device.hpp"

VulkanBuffer::VulkanBuffer(const BufferInfo& info, VmaAllocator allocator, VulkanDevice* device) :
    m_allocator(allocator), m_device(device)
{
  create(info);
}

VulkanBuffer::~VulkanBuffer() { destroy(); }

void VulkanBuffer::create(const BufferInfo& info)
{
  const std::unordered_set uniqueQueueFamilies{
      m_device->queueFamilies().graphics.value(), m_device->queueFamilies().compute.value(),
      m_device->queueFamilies().present.value(), m_device->queueFamilies().transfer.value()};
  const std::vector queueFamilies(uniqueQueueFamilies.begin(), uniqueQueueFamilies.end());

  vk::BufferCreateInfo bufferCreateInfo{.size = info.size,
                                        .usage = info.usage,
                                        .sharingMode = vk::SharingMode::eConcurrent,
                                        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size()),
                                        .pQueueFamilyIndices = queueFamilies.data()};

  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage = info.memoryUsage;
  vmaAllocInfo.flags = info.memoryFlags;

  const auto rawBufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);

  VmaAllocationInfo allocationInfo;

  VkBuffer tempBuffer = VK_NULL_HANDLE;
  VK_CHECK(vmaCreateBuffer(m_allocator, &rawBufferInfo, &vmaAllocInfo, &tempBuffer, &m_allocation, &allocationInfo));
  m_buffer = tempBuffer;
  m_size = info.size;
  m_mappedData = allocationInfo.pMappedData;
}

void VulkanBuffer::write(const void* srcData)
{
  if (m_mappedData != nullptr)
  {
    memcpy(m_mappedData, srcData, m_size);
    return;
  }

  void* data = map();
  memcpy(data, srcData, m_size);
  unmap();
}

void VulkanBuffer::writeRange(const void* srcData, const uint32_t rangeSize)
{
  if (m_mappedData != nullptr)
  {
    memcpy(m_mappedData, srcData, rangeSize);
    return;
  }

  void* data = map();
  memcpy(data, srcData, rangeSize);
  unmap();
}

void VulkanBuffer::writeRangeOffset(const void* srcData, const uint32_t rangeSize, const uint32_t offset)
{
  if (m_mappedData != nullptr)
  {
    memcpy(static_cast<uint8_t*>(m_mappedData) + offset, srcData, rangeSize);
    return;
  }

  void* data = map();
  memcpy(static_cast<uint8_t*>(data) + offset, srcData, rangeSize);
  unmap();
}

void* VulkanBuffer::map()
{
  if (m_mappedData != nullptr)
  {
    vmaMapMemory(m_allocator, m_allocation, &m_mappedData);
    return m_mappedData;
  }

  void* data = nullptr;
  vmaMapMemory(m_allocator, m_allocation, &data);

  return data;
}

void VulkanBuffer::unmap() const { vmaUnmapMemory(m_allocator, m_allocation); }

void VulkanBuffer::destroy()
{
  vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
  m_buffer = nullptr;
  m_allocation = VK_NULL_HANDLE;
  m_mappedData = nullptr;
}

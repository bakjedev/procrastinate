#include "vk_buffer.hpp"

#include <cstring>
#include <unordered_set>

#include "util/vk_check.hpp"
#include "vk_device.hpp"

VulkanBuffer::VulkanBuffer(const BufferInfo& info, VmaAllocator allocator, VulkanDevice* device) :
    allocator_(allocator), device_(device)
{
  Create(info);
}

VulkanBuffer::~VulkanBuffer() { Destroy(); }

void VulkanBuffer::Create(const BufferInfo& info)
{
  const std::unordered_set uniqueQueueFamilies{
      device_->QueueFamilies().graphics.value(), device_->QueueFamilies().compute.value(),
      device_->QueueFamilies().present.value(), device_->QueueFamilies().transfer.value()};
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
  VK_CHECK(vmaCreateBuffer(allocator_, &rawBufferInfo, &vmaAllocInfo, &tempBuffer, &allocation_, &allocationInfo));
  buffer_ = tempBuffer;
  size_ = info.size;
  mapped_data_ = allocationInfo.pMappedData;
}

void VulkanBuffer::Write(const void* src_data)
{
  if (mapped_data_ != nullptr)
  {
    memcpy(mapped_data_, src_data, size_);
    return;
  }

  void* data = map();
  memcpy(data, src_data, size_);
  unmap();
}

void VulkanBuffer::WriteRange(const void* src_data, const uint32_t range_size)
{
  if (mapped_data_ != nullptr)
  {
    memcpy(mapped_data_, src_data, range_size);
    return;
  }

  void* data = map();
  memcpy(data, src_data, range_size);
  unmap();
}

void VulkanBuffer::WriteRangeOffset(const void* src_data, const uint32_t range_size, const uint32_t offset)
{
  if (mapped_data_ != nullptr)
  {
    memcpy(static_cast<uint8_t*>(mapped_data_) + offset, src_data, range_size);
    return;
  }

  void* data = map();
  memcpy(static_cast<uint8_t*>(data) + offset, src_data, range_size);
  unmap();
}

void* VulkanBuffer::map()
{
  if (mapped_data_ != nullptr)
  {
    vmaMapMemory(allocator_, allocation_, &mapped_data_);
    return mapped_data_;
  }

  void* data = nullptr;
  vmaMapMemory(allocator_, allocation_, &data);

  return data;
}

void VulkanBuffer::unmap() const { vmaUnmapMemory(allocator_, allocation_); }

void VulkanBuffer::Destroy()
{
  vmaDestroyBuffer(allocator_, buffer_, allocation_);
  buffer_ = nullptr;
  allocation_ = VK_NULL_HANDLE;
  mapped_data_ = nullptr;
}

#include "vk_buffer.hpp"

#include <cstring>
#include <unordered_set>

#include "util/vk_check.hpp"
#include "vk_device.hpp"

VulkanBuffer::VulkanBuffer(const BufferInfo& info, const VmaAllocator allocator, VulkanDevice* device) :
    allocator_(allocator), device_(device)
{
  Create(info);
}

VulkanBuffer::~VulkanBuffer() { Destroy(); }

void VulkanBuffer::Create(const BufferInfo& info)
{
  const std::unordered_set unique_queue_families{
      device_->QueueFamilies().graphics.value(), device_->QueueFamilies().compute.value(),
      device_->QueueFamilies().present.value(), device_->QueueFamilies().transfer.value()};
  const std::vector queue_families(unique_queue_families.begin(), unique_queue_families.end());

  vk::BufferCreateInfo buffer_create_info{.size = info.size,
                                          .usage = info.usage,
                                          .sharingMode = vk::SharingMode::eConcurrent,
                                          .queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size()),
                                          .pQueueFamilyIndices = queue_families.data()};

  VmaAllocationCreateInfo vma_alloc_info = {};
  vma_alloc_info.usage = info.memoryUsage;
  vma_alloc_info.flags = info.memoryFlags;

  const auto raw_buffer_info = static_cast<VkBufferCreateInfo>(buffer_create_info);

  VmaAllocationInfo allocation_info;

  VkBuffer temp_buffer = VK_NULL_HANDLE;
  VK_CHECK(
      vmaCreateBuffer(allocator_, &raw_buffer_info, &vma_alloc_info, &temp_buffer, &allocation_, &allocation_info));
  buffer_ = temp_buffer;
  size_ = info.size;
  mapped_data_ = allocation_info.pMappedData;
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

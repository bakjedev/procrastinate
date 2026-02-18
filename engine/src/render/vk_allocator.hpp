#pragma once
#include <vulkan/vulkan.hpp>

#include "util/vk_check.hpp"
#include "vma/vma_usage.h"

class VulkanAllocator
{
public:
  VulkanAllocator(const vk::PhysicalDevice physical_device, const vk::Device device, const vk::Instance instance)
  {
    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = physical_device;
    info.device = device;
    info.instance = instance;
    info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateAllocator(&info, &allocator_));
  }

  ~VulkanAllocator() { vmaDestroyAllocator(allocator_); }

  VulkanAllocator(const VulkanAllocator &) = delete;
  VulkanAllocator(VulkanAllocator &&) = delete;
  VulkanAllocator &operator=(const VulkanAllocator &) = delete;
  VulkanAllocator &operator=(VulkanAllocator &&) = delete;

  [[nodiscard]] VmaAllocator get() const { return allocator_; }

private:
  VmaAllocator allocator_ = nullptr;
};

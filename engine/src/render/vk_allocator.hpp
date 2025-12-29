#pragma once
#include <vulkan/vulkan.hpp>

#include "util/vk_check.hpp"
#include "vma/vma_usage.h"

class VulkanAllocator {
 public:
  VulkanAllocator(const vk::PhysicalDevice physicalDevice,
                  const vk::Device device, const vk::Instance instance) {
    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = physicalDevice;
    info.device = device;
    info.instance = instance;
    info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateAllocator(&info, &m_allocator));
  }

  ~VulkanAllocator() { vmaDestroyAllocator(m_allocator); }

  VulkanAllocator(const VulkanAllocator &) = delete;
  VulkanAllocator(VulkanAllocator &&) = delete;
  VulkanAllocator &operator=(const VulkanAllocator &) = delete;
  VulkanAllocator &operator=(VulkanAllocator &&) = delete;

  [[nodiscard]] VmaAllocator get() const { return m_allocator; }

 private:
  VmaAllocator m_allocator = nullptr;
};
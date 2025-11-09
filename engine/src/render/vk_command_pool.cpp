#include "render/vk_command_pool.hpp"

#include "util/print.hpp"

VulkanCommandPool::VulkanCommandPool(const CommandPoolInfo& info,
                                     vk::Device device)
    : m_device(device) {
  create(info);
}

VulkanCommandPool::~VulkanCommandPool() { destroy(); }

vk::CommandBuffer VulkanCommandPool::allocate(
    const vk::CommandBufferLevel level) const {
  vk::CommandBufferAllocateInfo allocateInfo{
      .commandPool = m_commandPool, .level = level, .commandBufferCount = 1};

  auto buffers = m_device.allocateCommandBuffers(allocateInfo);
  Util::println("Allocated command buffer");
  return buffers[0];
}

void VulkanCommandPool::free(vk::CommandBuffer commandBuffer) const {
  m_device.freeCommandBuffers(m_commandPool, 1, &commandBuffer);
}

void VulkanCommandPool::reset() const {
  m_device.resetCommandPool(m_commandPool, vk::CommandPoolResetFlags{});
}

void VulkanCommandPool::create(const CommandPoolInfo& info) {
  vk::CommandPoolCreateInfo createInfo{
      .flags = info.flags, .queueFamilyIndex = info.queueFamilyIndex};

  m_commandPool = m_device.createCommandPool(createInfo);
  Util::println("Created command pool with queue family index {}",
                info.queueFamilyIndex);
}

void VulkanCommandPool::destroy() const {
  m_device.destroyCommandPool(m_commandPool);
  Util::println("Destroyed command pool");
}
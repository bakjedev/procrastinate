#include "render/vk_command_pool.hpp"

#include "util/print.hpp"

VulkanCommandPool::VulkanCommandPool(const CommandPoolInfo& info, const vk::Device device) : device_(device)
{
  create(info);
}

VulkanCommandPool::~VulkanCommandPool() { destroy(); }

vk::CommandBuffer VulkanCommandPool::allocate(const vk::CommandBufferLevel level) const
{
  const vk::CommandBufferAllocateInfo allocateInfo{
      .commandPool = command_pool_, .level = level, .commandBufferCount = 1};

  const auto buffers = device_.allocateCommandBuffers(allocateInfo);
  return buffers.front();
}

void VulkanCommandPool::free(const vk::CommandBuffer command_buffer) const
{
  device_.freeCommandBuffers(command_pool_, 1, &command_buffer);
}

void VulkanCommandPool::reset() const { device_.resetCommandPool(command_pool_, vk::CommandPoolResetFlags{}); }

void VulkanCommandPool::create(const CommandPoolInfo& info)
{
  const vk::CommandPoolCreateInfo createInfo{.flags = info.flags, .queueFamilyIndex = info.queue_family_index};

  command_pool_ = device_.createCommandPool(createInfo);
}

void VulkanCommandPool::destroy() const { device_.destroyCommandPool(command_pool_); }

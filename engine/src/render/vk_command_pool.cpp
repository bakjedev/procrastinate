#include "render/vk_command_pool.hpp"

#include "util/util.hpp"

VulkanCommandPool::VulkanCommandPool(VkDevice device,
                                     const int queueFamilyIndex,
                                     const VkCommandPoolCreateFlags flags)
    : m_device(device) {
  create(queueFamilyIndex,
         VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | flags);
}

VulkanCommandPool::~VulkanCommandPool() { destroy(); }

VkCommandBuffer VulkanCommandPool::allocate(
    const VkCommandBufferLevel level) const {
  VkCommandBuffer buffer;
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = m_commandPool;
  allocateInfo.commandBufferCount = 1;
  allocateInfo.level = level;
  VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, &buffer));
  Util::println("Allocated command buffer");
  return buffer;
}

void VulkanCommandPool::free(VkCommandBuffer commandBuffer) const {
  vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanCommandPool::reset() const {
  VK_CHECK(vkResetCommandPool(m_device, m_commandPool, 0));
}

void VulkanCommandPool::create(const uint32_t queueFamilyIndex,
                               const VkCommandPoolCreateFlags flags) {
  VkCommandPoolCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = queueFamilyIndex;
  createInfo.flags = flags;

  VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool));
  Util::println("Created command pool with queue family index {}",
                queueFamilyIndex);
}

void VulkanCommandPool::destroy() const {
  vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  Util::println("Destroyed command pool");
}

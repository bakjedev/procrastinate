#pragma once
#include <vulkan/vulkan.h>

class VulkanCommandPool {
 public:
  explicit VulkanCommandPool(VkDevice device, int queueFamilyIndex,
                             VkCommandPoolCreateFlags flags);
  ~VulkanCommandPool();

  [[nodiscard]] VkCommandPool get() const { return m_commandPool; }
  [[nodiscard]] VkCommandBuffer allocate(
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
  void free(VkCommandBuffer commandBuffer) const;
  void reset() const;

 private:
  VkCommandPool m_commandPool{};
  VkDevice m_device{};  // for deconstructing, allocating and resetting

  void create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0U);
  void destroy() const;
};
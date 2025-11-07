#pragma once
#include <vulkan/vulkan.h>

struct CommandPoolInfo {
  uint32_t queueFamilyIndex{};
  VkCommandPoolCreateFlags flags{};
};

class VulkanCommandPool {
 public:
  explicit VulkanCommandPool(const CommandPoolInfo& info, VkDevice device);
  VulkanCommandPool(const VulkanCommandPool&) = delete;
  VulkanCommandPool(VulkanCommandPool&&) = delete;
  VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
  VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;
  ~VulkanCommandPool();

  [[nodiscard]] VkCommandPool get() const { return m_commandPool; }
  [[nodiscard]] VkCommandBuffer allocate(
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;
  void free(VkCommandBuffer commandBuffer) const;
  void reset() const;

 private:
  VkCommandPool m_commandPool{};
  VkDevice m_device{};  // for deconstructing, allocating and resetting

  void create(const CommandPoolInfo& info);
  void destroy() const;
};
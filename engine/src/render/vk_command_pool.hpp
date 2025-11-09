#pragma once
#include <vulkan/vulkan.hpp>

struct CommandPoolInfo {
  uint32_t queueFamilyIndex{};
  vk::CommandPoolCreateFlags flags;
};

class VulkanCommandPool {
 public:
  explicit VulkanCommandPool(const CommandPoolInfo& info, vk::Device device);
  VulkanCommandPool(const VulkanCommandPool&) = delete;
  VulkanCommandPool(VulkanCommandPool&&) = delete;
  VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
  VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;
  ~VulkanCommandPool();

  [[nodiscard]] vk::CommandPool get() const { return m_commandPool; }
  vk::CommandBuffer allocate(
      vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
  void free(vk::CommandBuffer commandBuffer) const;
  void reset() const;

 private:
  vk::Device m_device;
  vk::CommandPool m_commandPool;

  void create(const CommandPoolInfo& info);
  void destroy() const;
};
#pragma once
#include <vulkan/vulkan.hpp>

struct CommandPoolInfo
{
  uint32_t queue_family_index{};
  vk::CommandPoolCreateFlags flags;
};

class VulkanCommandPool
{
public:
  explicit VulkanCommandPool(const CommandPoolInfo& info, vk::Device device);
  VulkanCommandPool(const VulkanCommandPool&) = delete;
  VulkanCommandPool(VulkanCommandPool&&) = delete;
  VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
  VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;
  ~VulkanCommandPool();

  [[nodiscard]] vk::CommandPool get() const { return command_pool_; }
  [[nodiscard]] vk::CommandBuffer allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
  void free(vk::CommandBuffer command_buffer) const;
  void reset() const;

private:
  vk::Device device_;
  vk::CommandPool command_pool_;

  void create(const CommandPoolInfo& info);
  void destroy() const;
};

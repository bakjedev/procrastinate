#pragma once
#include <vulkan/vulkan.hpp>

class VulkanCommandPool;

class VulkanFrame {
 public:
  VulkanFrame(VulkanCommandPool* graphicsPool, VulkanCommandPool* transferPool,
              VulkanCommandPool* computePool, vk::Device device);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer graphicsCmd() const { return m_graphicsCmd; }
  [[nodiscard]] vk::CommandBuffer transferCmd() const { return m_transferCmd; }
  [[nodiscard]] vk::CommandBuffer computeCmd() const { return m_computeCmd; }
  [[nodiscard]] vk::Semaphore renderFinished() const {
    return m_renderFinished;
  }

 private:
  vk::CommandBuffer m_graphicsCmd;
  vk::CommandBuffer m_transferCmd;
  vk::CommandBuffer m_computeCmd;

  vk::Semaphore m_renderFinished;

  vk::Device m_device;
};
#pragma once
#include <vulkan/vulkan.h>

class VulkanCommandPool;

class VulkanFrame {
 public:
  VulkanFrame(VulkanCommandPool* graphicsPool, VulkanCommandPool* transferPool,
              VulkanCommandPool* computePool, VkDevice device);
  ~VulkanFrame();

  VkCommandBuffer graphics() const { return m_graphicsCmd; }
  VkCommandBuffer transfer() const { return m_transferCmd; }
  VkCommandBuffer compute() const { return m_computeCmd; }

  VkSemaphore imageAvailable() const { return m_imageAvailable; }
  VkSemaphore renderFinished() const { return m_renderFinished; }
  VkFence inFlight() const { return m_inFlight; }

 private:
  VkCommandBuffer m_graphicsCmd;
  VkCommandBuffer m_transferCmd;
  VkCommandBuffer m_computeCmd;
  VkSemaphore m_imageAvailable;
  VkSemaphore m_renderFinished;
  VkFence m_inFlight;

  VkDevice m_device;  // for deconstructing
};
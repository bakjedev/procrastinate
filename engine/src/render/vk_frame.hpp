#pragma once
#include <vulkan/vulkan.h>

class VulkanCommandPool;

class VulkanFrame {
 public:
  VulkanFrame(VulkanCommandPool* graphicsPool, VulkanCommandPool* transferPool,
              VulkanCommandPool* computePool, VkDevice device);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] VkCommandBuffer graphics() const { return m_graphicsCmd; }
  [[nodiscard]] VkCommandBuffer transfer() const { return m_transferCmd; }
  [[nodiscard]] VkCommandBuffer compute() const { return m_computeCmd; }

  [[nodiscard]] VkSemaphore imageAvailable() const { return m_imageAvailable; }
  [[nodiscard]] VkSemaphore renderFinished() const { return m_renderFinished; }
  [[nodiscard]] const VkFence& inFlight() const { return m_inFlight; }

 private:
  VkCommandBuffer m_graphicsCmd{};
  VkCommandBuffer m_transferCmd{};
  VkCommandBuffer m_computeCmd{};
  VkSemaphore m_imageAvailable{};
  VkSemaphore m_renderFinished{};
  VkFence m_inFlight{};

  VkDevice m_device;  // for deconstructing
};
#pragma once

#include "render/vk_buffer.hpp"
#include "vulkan/vulkan.hpp"

class VulkanCommandPool;
class VulkanAllocator;

class VulkanFrame {
 public:
  VulkanFrame(VulkanCommandPool* graphicsPool, VulkanCommandPool* transferPool,
              VulkanCommandPool* computePool, vk::Device device,
              VulkanAllocator* allocator);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer graphicsCmd() const { return m_graphicsCmd; }
  [[nodiscard]] vk::CommandBuffer transferCmd() const { return m_transferCmd; }
  [[nodiscard]] vk::CommandBuffer computeCmd() const { return m_computeCmd; }
  [[nodiscard]] VulkanBuffer* stagingBuffer() const {
    return m_stagingBuffer.get();
  }
  [[nodiscard]] VulkanBuffer* indirectBuffer() const {
    return m_indirectBuffer.get();
  }

  [[nodiscard]] vk::Semaphore renderFinished() const {
    return m_renderFinished;
  }

 private:
  vk::CommandBuffer m_graphicsCmd;
  vk::CommandBuffer m_transferCmd;
  vk::CommandBuffer m_computeCmd;

  vk::Semaphore m_renderFinished;

  std::unique_ptr<VulkanBuffer> m_indirectBuffer;
  std::unique_ptr<VulkanBuffer> m_stagingBuffer;
  vk::Device m_device;
};
#pragma once

#include <memory>

#include "glm/glm.hpp"
#include "render/vk_buffer.hpp"
#include "render/vk_descriptor.hpp"
#include "vulkan/vulkan.hpp"

class VulkanCommandPool;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;
class VulkanAllocator;

struct RenderObject {
  glm::mat4 model;
  uint32_t meshID;
  int32_t padding[3];
};

class VulkanFrame {
 public:
  VulkanFrame(VulkanCommandPool* graphicsPool, VulkanCommandPool* transferPool,
              VulkanCommandPool* computePool,
              VulkanDescriptorPool* descriptorPool,
              VulkanDescriptorSetLayout* descriptorLayout, vk::Device device,
              VulkanAllocator* allocator);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer graphicsCmd() const { return m_graphicsCmd; }
  [[nodiscard]] vk::CommandBuffer transferCmd() const { return m_transferCmd; }
  [[nodiscard]] vk::CommandBuffer computeCmd() const { return m_computeCmd; }

  [[nodiscard]] VulkanBuffer* objectBuffer() const {
    return m_objectBuffer.get();
  }

  [[nodiscard]] vk::Semaphore imageAvailable() const {
    return m_imageAvailable.get();
  }

  [[nodiscard]] vk::Fence inFlight() const { return m_inFlight.get(); }

  [[nodiscard]] vk::Semaphore computeFinished() const {
    return m_computeFinished.get();
  }

  [[nodiscard]] vk::DescriptorSet& descriptorSet() { return m_descriptorSet; }

 private:
  vk::CommandBuffer m_graphicsCmd;
  vk::CommandBuffer m_transferCmd;
  vk::CommandBuffer m_computeCmd;

  vk::UniqueSemaphore m_imageAvailable;
  vk::UniqueFence m_inFlight;
  vk::UniqueSemaphore m_computeFinished;

  std::unique_ptr<VulkanBuffer> m_objectBuffer;
  vk::DescriptorSet m_descriptorSet;

  vk::Device m_device;
};

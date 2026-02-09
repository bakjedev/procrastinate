#pragma once

#include <memory>

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

class VulkanImage;
class VulkanBuffer;
class VulkanCommandPool;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;
class VulkanAllocator;

struct RenderObject {
  glm::mat4 model;
  uint32_t meshID;
  std::array<int32_t, 3> pad;
};

struct DebugLineVertex {
  glm::vec3 position;
  glm::vec3 color;
};

class VulkanFrame {
 public:
  VulkanFrame(const VulkanCommandPool* graphicsPool,
              const VulkanCommandPool* transferPool,
              const VulkanCommandPool* computePool,
              const VulkanDescriptorPool* descriptorPool,
              const VulkanDescriptorSetLayout* descriptorLayout,
              vk::Device device, VulkanAllocator* allocator);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer graphicsCmd() const { return m_graphicsCmd; }
  [[nodiscard]] vk::CommandBuffer transferCmd() const { return m_transferCmd; }
  [[nodiscard]] vk::CommandBuffer computeCmd() const { return m_computeCmd; }

  [[nodiscard]] VulkanImage* depthImage() const { return m_depthImage.get(); }

  [[nodiscard]] VulkanBuffer* objectBuffer() const {
    return m_objectBuffer.get();
  }

  [[nodiscard]] VulkanBuffer* indirectBuffer() const {
    return m_indirectBuffer.get();
  }

  [[nodiscard]] VulkanBuffer* drawCount() const { return m_drawCount.get(); }

  [[nodiscard]] VulkanBuffer* debugLineVertexBuffer() const {
    return m_debugLineVertexBuffer.get();
  }

  [[nodiscard]] vk::Semaphore imageAvailable() const {
    return m_imageAvailable.get();
  }

  [[nodiscard]] vk::Fence inFlight() const { return m_inFlight.get(); }

  [[nodiscard]] vk::Semaphore computeFinished() const {
    return m_computeFinished.get();
  }

  [[nodiscard]] vk::DescriptorSet& descriptorSet() { return m_descriptorSet; }

  void recreateDepthImage(const uint32_t width, const uint32_t height);

 private:
  vk::CommandBuffer m_graphicsCmd;
  vk::CommandBuffer m_transferCmd;
  vk::CommandBuffer m_computeCmd;

  vk::UniqueSemaphore m_imageAvailable;
  vk::UniqueFence m_inFlight;
  vk::UniqueSemaphore m_computeFinished;

  std::unique_ptr<VulkanImage> m_depthImage;

  std::unique_ptr<VulkanBuffer> m_objectBuffer;
  std::unique_ptr<VulkanBuffer> m_indirectBuffer;
  std::unique_ptr<VulkanBuffer> m_drawCount;
  vk::DescriptorSet m_descriptorSet;

  std::unique_ptr<VulkanBuffer> m_debugLineVertexBuffer;

  vk::Device m_device;
  VulkanAllocator* m_allocator;
};

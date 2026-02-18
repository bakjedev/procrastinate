#pragma once

#include <memory>

#include "vulkan/vulkan.hpp"

class VulkanImage;
class VulkanBuffer;
class VulkanCommandPool;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;
class VulkanAllocator;
class VulkanDevice;

class VulkanFrame
{
public:
  VulkanFrame(const VulkanCommandPool* graphicsPool, const VulkanCommandPool* computePool,
              const VulkanDescriptorPool* descriptorPool, const VulkanDescriptorSetLayout* descriptorLayout,
              VulkanDevice* device, VulkanAllocator* allocator);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer graphicsCmd() const { return m_graphicsCmd; }
  [[nodiscard]] vk::CommandBuffer computeCmd() const { return m_computeCmd; }

  [[nodiscard]] VulkanImage* depthImage() const { return m_depthImage.get(); }
  [[nodiscard]] VulkanImage* visibilityImage() const { return m_visibilityImage.get(); }
  [[nodiscard]] VulkanImage* renderImage() const { return m_renderImage.get(); }

  [[nodiscard]] VulkanBuffer* objectBuffer() const { return m_objectBuffer.get(); }

  [[nodiscard]] VulkanBuffer* indirectBuffer() const { return m_indirectBuffer.get(); }

  [[nodiscard]] VulkanBuffer* drawCount() const { return m_drawCount.get(); }

  [[nodiscard]] VulkanBuffer* debugLineVertexBuffer() const { return m_debugLineVertexBuffer.get(); }

  [[nodiscard]] vk::Semaphore imageAvailable() const { return m_imageAvailable.get(); }

  [[nodiscard]] vk::Fence inFlight() const { return m_inFlight.get(); }

  [[nodiscard]] vk::Semaphore computeFinished() const { return m_computeFinished.get(); }

  [[nodiscard]] vk::DescriptorSet& descriptorSet() { return m_descriptorSet; }

  void recreateFrameImages(const uint32_t width, const uint32_t height);

private:
  vk::CommandBuffer m_graphicsCmd;
  vk::CommandBuffer m_computeCmd;

  vk::UniqueSemaphore m_imageAvailable;
  vk::UniqueFence m_inFlight;
  vk::UniqueSemaphore m_computeFinished;

  std::unique_ptr<VulkanImage> m_visibilityImage;
  std::unique_ptr<VulkanImage> m_depthImage;
  std::unique_ptr<VulkanImage> m_renderImage;

  std::unique_ptr<VulkanBuffer> m_objectBuffer;
  std::unique_ptr<VulkanBuffer> m_indirectBuffer;
  std::unique_ptr<VulkanBuffer> m_drawCount;
  vk::DescriptorSet m_descriptorSet;

  std::unique_ptr<VulkanBuffer> m_debugLineVertexBuffer;

  VulkanDevice* m_device;
  VulkanAllocator* m_allocator;
};

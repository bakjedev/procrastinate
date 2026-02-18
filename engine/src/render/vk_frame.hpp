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
  VulkanFrame(const VulkanCommandPool* graphics_pool, const VulkanCommandPool* compute_pool,
              const VulkanDescriptorPool* descriptor_pool, const VulkanDescriptorSetLayout* descriptor_layout,
              VulkanDevice* device, VulkanAllocator* allocator);
  VulkanFrame(const VulkanFrame&) = delete;
  VulkanFrame(VulkanFrame&&) = delete;
  VulkanFrame& operator=(const VulkanFrame&) = delete;
  VulkanFrame& operator=(VulkanFrame&&) = delete;
  ~VulkanFrame();

  [[nodiscard]] vk::CommandBuffer GraphicsCmd() const { return graphics_cmd_; }
  [[nodiscard]] vk::CommandBuffer ComputeCmd() const { return compute_cmd_; }

  [[nodiscard]] VulkanImage* DepthImage() const { return depth_image_.get(); }
  [[nodiscard]] VulkanImage* VisibilityImage() const { return visibility_image_.get(); }
  [[nodiscard]] VulkanImage* RenderImage() const { return render_image_.get(); }

  [[nodiscard]] VulkanBuffer* ObjectBuffer() const { return object_buffer_.get(); }

  [[nodiscard]] VulkanBuffer* IndirectBuffer() const { return indirect_buffer_.get(); }

  [[nodiscard]] VulkanBuffer* DrawCount() const { return draw_count_.get(); }

  [[nodiscard]] VulkanBuffer* DebugLineVertexBuffer() const { return debug_line_vertex_buffer_.get(); }

  [[nodiscard]] vk::Semaphore ImageAvailable() const { return image_available_.get(); }

  [[nodiscard]] vk::Fence InFlight() const { return in_flight_.get(); }

  [[nodiscard]] vk::Semaphore ComputeFinished() const { return compute_finished_.get(); }

  [[nodiscard]] vk::DescriptorSet& DescriptorSet() { return descriptor_set_; }

  void RecreateFrameImages(const uint32_t width, const uint32_t height);

private:
  vk::CommandBuffer graphics_cmd_;
  vk::CommandBuffer compute_cmd_;

  vk::UniqueSemaphore image_available_;
  vk::UniqueFence in_flight_;
  vk::UniqueSemaphore compute_finished_;

  std::unique_ptr<VulkanImage> visibility_image_;
  std::unique_ptr<VulkanImage> depth_image_;
  std::unique_ptr<VulkanImage> render_image_;

  std::unique_ptr<VulkanBuffer> object_buffer_;
  std::unique_ptr<VulkanBuffer> indirect_buffer_;
  std::unique_ptr<VulkanBuffer> draw_count_;
  vk::DescriptorSet descriptor_set_;

  std::unique_ptr<VulkanBuffer> debug_line_vertex_buffer_;

  VulkanDevice* device_;
  VulkanAllocator* allocator_;
};

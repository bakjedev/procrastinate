#pragma once
#include <vma/vma_usage.h>

#include <vulkan/vulkan.hpp>

struct ImageInfo
{
  uint32_t width;
  uint32_t height;
  vk::Format format;
  vk::ImageUsageFlags usage;
  vk::ImageAspectFlags aspect_flags;
};

class VulkanImage
{
public:
  VulkanImage(const ImageInfo& info, VmaAllocator allocator);
  VulkanImage(const VulkanImage&) = delete;
  VulkanImage(VulkanImage&&) = delete;
  VulkanImage& operator=(const VulkanImage&) = delete;
  VulkanImage& operator=(VulkanImage&&) = delete;
  ~VulkanImage();

  [[nodiscard]] vk::Image get() const { return image_; }

  [[nodiscard]] vk::ImageView view() const { return image_view_; }
  [[nodiscard]] VmaAllocation allocation() const { return allocation_; }
  [[nodiscard]] vk::Format format() const { return format_; }
  [[nodiscard]] uint32_t width() const { return width_; }
  [[nodiscard]] uint32_t height() const { return height_; }

  void TransitionLayout(vk::CommandBuffer cmd, vk::ImageLayout old_layout, vk::ImageLayout new_layout) const;

  static void TransitionImageLayout(vk::Image image, vk::CommandBuffer cmd, vk::ImageLayout old_layout,
                                    vk::ImageLayout new_layout);
  void Destroy();

private:
  vk::Image image_;
  vk::ImageView image_view_;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  vk::Format format_;
  uint32_t width_;
  uint32_t height_;

  VmaAllocator allocator_;
  vk::Device device_;

  void Create(const ImageInfo& info);
  void CreateView(vk::ImageAspectFlags aspect_flags);
};

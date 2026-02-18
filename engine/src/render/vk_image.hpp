#pragma once
#include <vma/vma_usage.h>

#include <vulkan/vulkan.hpp>

struct ImageInfo
{
  uint32_t width;
  uint32_t height;
  vk::Format format;
  vk::ImageUsageFlags usage;
  vk::ImageAspectFlags aspectFlags;
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

  [[nodiscard]] vk::Image get() const { return m_image; }

  [[nodiscard]] vk::ImageView view() const { return m_imageView; }
  [[nodiscard]] VmaAllocation allocation() const { return m_allocation; }
  [[nodiscard]] vk::Format format() const { return m_format; }
  [[nodiscard]] uint32_t width() const { return m_width; }
  [[nodiscard]] uint32_t height() const { return m_height; }

  void transitionLayout(vk::CommandBuffer cmd, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;

  static void transitionImageLayout(vk::Image image, vk::CommandBuffer cmd, vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout);
  void destroy();

private:
  vk::Image m_image;
  vk::ImageView m_imageView;
  VmaAllocation m_allocation = VK_NULL_HANDLE;

  vk::Format m_format;
  uint32_t m_width;
  uint32_t m_height;

  VmaAllocator m_allocator;
  vk::Device m_device;

  void create(const ImageInfo& info);
  void createView(vk::ImageAspectFlags aspectFlags);
};

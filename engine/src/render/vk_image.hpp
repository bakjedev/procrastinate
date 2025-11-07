#pragma once
#include <vma/vma_usage.h>

struct ImageInfo {
  uint32_t width;
  uint32_t height;
  VkFormat format;
  VkImageUsageFlags usage;
  VkImageAspectFlags aspectFlags;
};

class VulkanImage {
 public:
  VulkanImage(const ImageInfo& info, VmaAllocator allocator);
  VulkanImage(const VulkanImage&) = delete;
  VulkanImage(VulkanImage&&) = delete;
  VulkanImage& operator=(const VulkanImage&) = delete;
  VulkanImage& operator=(VulkanImage&&) = delete;
  ~VulkanImage();

  [[nodiscard]] VkImage get() const { return m_image; }

  [[nodiscard]] VkImageView view() const { return m_imageView; }
  [[nodiscard]] VmaAllocation allocation() const { return m_allocation; }
  [[nodiscard]] VkFormat format() const { return m_format; }
  [[nodiscard]] uint32_t width() const { return m_width; }
  [[nodiscard]] uint32_t height() const { return m_height; }

  void transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout,
                        VkImageLayout newLayout);

  static void transitionImageLayout(VkImage image, VkCommandBuffer cmd,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout);

 private:
  VkImage m_image = VK_NULL_HANDLE;
  VkImageView m_imageView = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;

  VkFormat m_format;
  uint32_t m_width;
  uint32_t m_height;

  VmaAllocator m_allocator;
  VkDevice m_device;

  void create(const ImageInfo& info);
  void createView(VkImageAspectFlags aspectFlags);
  void destroy();
};
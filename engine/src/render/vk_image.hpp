#pragma once
#include <vma/vma_usage.h>

class VulkanImage {
 public:
  VulkanImage(VmaAllocator allocator, uint32_t width, uint32_t height,
              VkFormat format, VkImageUsageFlags usage,
              VkImageAspectFlags aspectFlags);
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

  VkImageAspectFlags m_aspectFlags;

  VkFormat m_format;
  uint32_t m_width;
  uint32_t m_height;

  VmaAllocator m_allocator;
  VkDevice m_device;

  void create(uint32_t width, uint32_t height, VkFormat format,
              VkImageUsageFlags usage);
  void createView();
  void destroy();
};
#include "vk_image.hpp"

#include "util/vk_check.hpp"

VulkanImage::VulkanImage(VmaAllocator allocator, uint32_t width,
                         uint32_t height, VkFormat format,
                         VkImageUsageFlags usage,
                         VkImageAspectFlags aspectFlags)
    : m_aspectFlags(aspectFlags),
      m_format(format),
      m_width(width),
      m_height(height),
      m_allocator(allocator) {
  VmaAllocatorInfo allocatorInfo;
  vmaGetAllocatorInfo(m_allocator, &allocatorInfo);
  m_device = allocatorInfo.device;

  create(width, height, format, usage);
  createView();
  Util::println("Created vulkan image ({}x{}, format {})", width, height,
                static_cast<int>(format));
}

VulkanImage::~VulkanImage() { destroy(); }

void VulkanImage::create(uint32_t width, uint32_t height, VkFormat format,
                         VkImageUsageFlags usage) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image,
                          &m_allocation, nullptr));
}

void VulkanImage::createView() {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = m_image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = m_format;
  viewInfo.subresourceRange.aspectMask = m_aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView));
}

void VulkanImage::destroy() {
  if (m_imageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device, m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }
  if (m_image != VK_NULL_HANDLE) {
    vmaDestroyImage(m_allocator, m_image, m_allocation);
    m_image = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
  }
  Util::println("Destroyed vulkan image");
}

void VulkanImage::transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout,
                                   VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = m_image;
  barrier.subresourceRange.aspectMask = m_aspectFlags;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else {
    throw std::runtime_error("Unsupported layout transition");
  }

  vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

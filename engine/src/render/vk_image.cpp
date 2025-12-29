#include "vk_image.hpp"

#include "util/vk_check.hpp"

VulkanImage::VulkanImage(const ImageInfo &info, VmaAllocator allocator)
    : m_format(info.format),
      m_width(info.width),
      m_height(info.height),
      m_allocator(allocator) {
  VmaAllocatorInfo allocatorInfo;
  vmaGetAllocatorInfo(m_allocator, &allocatorInfo);
  m_device = allocatorInfo.device;

  create(info);
  createView(info.aspectFlags);
}

VulkanImage::~VulkanImage() { destroy(); }

void VulkanImage::transitionLayout(const vk::CommandBuffer cmd,
                                   const vk::ImageLayout oldLayout,
                                   const vk::ImageLayout newLayout) const {
  transitionImageLayout(m_image, cmd, oldLayout, newLayout);
}

// static
void VulkanImage::transitionImageLayout(const vk::Image image,
                                        const vk::CommandBuffer cmd,
                                        const vk::ImageLayout oldLayout,
                                        const vk::ImageLayout newLayout) {
  const vk::ImageAspectFlags aspectMask =
      (newLayout == vk::ImageLayout::eDepthAttachmentOptimal)
          ? vk::ImageAspectFlagBits::eDepth
          : vk::ImageAspectFlagBits::eColor;

  vk::ImageMemoryBarrier2 barrier{
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          vk::ImageSubresourceRange{.aspectMask = aspectMask,
                                    .baseMipLevel = 0,
                                    .levelCount = VK_REMAINING_MIP_LEVELS,
                                    .baseArrayLayer = 0,
                                    .layerCount = VK_REMAINING_ARRAY_LAYERS}};

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
             newLayout == vk::ImageLayout::ePresentSrcKHR) {
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
  } else {
    throw std::runtime_error("Unsupported layout transition");
  }

  const vk::DependencyInfo depInfo{.imageMemoryBarrierCount = 1,
                                   .pImageMemoryBarriers = &barrier};

  cmd.pipelineBarrier2(depInfo);
}

void VulkanImage::destroy() {
  if (m_imageView) {
    m_device.destroyImageView(m_imageView);
    m_imageView = nullptr;
  }
  if (m_image) {
    vmaDestroyImage(m_allocator, static_cast<VkImage>(m_image), m_allocation);
    m_image = nullptr;
    m_allocation = VK_NULL_HANDLE;
  }
}

void VulkanImage::create(const ImageInfo &info) {
  vk::ImageCreateInfo imageInfo{
      .imageType = vk::ImageType::e2D,
      .format = info.format,
      .extent =
          vk::Extent3D{.width = info.width, .height = info.height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = info.usage,
      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined};

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  const auto rawImageInfo = static_cast<VkImageCreateInfo>(imageInfo);

  VkImage tempImage = VK_NULL_HANDLE;
  VK_CHECK(vmaCreateImage(m_allocator, &rawImageInfo, &allocInfo, &tempImage,
                          &m_allocation, nullptr));
  m_image = tempImage;
}

void VulkanImage::createView(const vk::ImageAspectFlags aspectFlags) {
  const vk::ImageViewCreateInfo viewInfo{
      .image = m_image,
      .viewType = vk::ImageViewType::e2D,
      .format = m_format,
      .subresourceRange = vk::ImageSubresourceRange{.aspectMask = aspectFlags,
                                                    .baseMipLevel = 0,
                                                    .levelCount = 1,
                                                    .baseArrayLayer = 0,
                                                    .layerCount = 1}};

  m_imageView = m_device.createImageView(viewInfo);
}


#include "render/vk_frame.hpp"

#include "render/vk_command_pool.hpp"
#include "render/vk_descriptor.hpp"
#include "render/vk_device.hpp"
#include "vk_allocator.hpp"
#include "vk_buffer.hpp"
#include "vk_image.hpp"
#include "vk_renderer.hpp"

constexpr uint32_t MAX_INDIRECT_COMMANDS = 65536;
constexpr uint32_t MAX_LINES = 10000;
constexpr uint32_t MAX_OBJECTS = 10000;

VulkanFrame::VulkanFrame(const VulkanCommandPool* graphicsPool,
                         const VulkanCommandPool* computePool,
                         const VulkanDescriptorPool* descriptorPool,
                         const VulkanDescriptorSetLayout* descriptorLayout,
                         VulkanDevice* device, VulkanAllocator* allocator) {
  // Set references
  m_device = device;
  m_allocator = allocator;

  // Create per frame sync objects
  constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
  constexpr vk::FenceCreateInfo fenceCreateInfo{
      .flags = vk::FenceCreateFlagBits::eSignaled};

  const auto internalDevice = m_device->get();
  m_imageAvailable = internalDevice.createSemaphoreUnique(semaphoreCreateInfo);
  m_inFlight = internalDevice.createFenceUnique(fenceCreateInfo);
  m_computeFinished = internalDevice.createSemaphoreUnique(semaphoreCreateInfo);

  // Allocate command buffers for graphics, transfer and compute.
  m_graphicsCmd = graphicsPool->allocate();
  m_computeCmd = computePool->allocate();

  // Create render object buffer
  m_objectBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{
          .size = sizeof(RenderObject) * MAX_OBJECTS,
          .usage = vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eTransferDst,
          .memoryUsage = VMA_MEMORY_USAGE_AUTO,
          .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT

      },
      allocator->get(), m_device);
  m_objectBuffer->map();

  m_indirectBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(vk::DrawIndexedIndirectCommand) *
                         MAX_INDIRECT_COMMANDS,
                 .usage = vk::BufferUsageFlagBits::eIndirectBuffer |
                          vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eStorageBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get(), m_device);

  // Create debug line vertex buffer
  m_debugLineVertexBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(DebugLineVertex) * 2 * MAX_LINES,
                 .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                 .memoryFlags =
                     VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT},
      allocator->get(), m_device);
  m_debugLineVertexBuffer->map();

  // Create draw count buffer
  m_drawCount = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(uint32_t),
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer |
                          vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eTransferSrc |
                          vk::BufferUsageFlagBits::eIndirectBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get(), m_device);

  // Allocate descriptor set with per frame descriptor set layout
  m_descriptorSet = descriptorPool->allocate(descriptorLayout->get());
}

VulkanFrame::~VulkanFrame() {
  m_objectBuffer->unmap();
  m_debugLineVertexBuffer->unmap();
}
void VulkanFrame::recreateFrameImages(const uint32_t width,
                                      const uint32_t height) {
  m_depthImage = nullptr;

  const ImageInfo depthImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eD32Sfloat,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .aspectFlags = vk::ImageAspectFlagBits::eDepth,
  };
  m_depthImage =
      std::make_unique<VulkanImage>(depthImageInfo, m_allocator->get());

  m_visibilityImage = nullptr;
  const ImageInfo visibilityImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eR32Uint,
      .usage = vk::ImageUsageFlagBits::eColorAttachment |
               vk::ImageUsageFlagBits::eSampled,
      .aspectFlags = vk::ImageAspectFlagBits::eColor,
  };
  m_visibilityImage =
      std::make_unique<VulkanImage>(visibilityImageInfo, m_allocator->get());
}

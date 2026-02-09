
#include "render/vk_frame.hpp"

#include <memory>
#include <vulkan/vulkan_structs.hpp>

#include "render/vk_command_pool.hpp"
#include "render/vk_descriptor.hpp"
#include "vk_allocator.hpp"
#include "vk_buffer.hpp"
#include "vk_image.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"

constexpr uint32_t MAX_INDIRECT_COMMANDS = 65536;
constexpr uint32_t MAX_LINES = 10000;
constexpr uint32_t MAX_OBJECTS = 10000;

VulkanFrame::VulkanFrame(const VulkanCommandPool* graphicsPool,
                         const VulkanCommandPool* transferPool,
                         const VulkanCommandPool* computePool,
                         const VulkanDescriptorPool* descriptorPool,
                         const VulkanDescriptorSetLayout* descriptorLayout,
                         const vk::Device device, VulkanAllocator* allocator) {
  // Set references
  m_device = device;
  m_allocator = allocator;

  // Create per frame sync objects
  constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
  constexpr vk::FenceCreateInfo fenceCreateInfo{
      .flags = vk::FenceCreateFlagBits::eSignaled};

  m_imageAvailable = m_device.createSemaphoreUnique(semaphoreCreateInfo);
  m_inFlight = m_device.createFenceUnique(fenceCreateInfo);
  m_computeFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);

  // Allocate command buffers for graphics, transfer and compute.
  m_graphicsCmd = graphicsPool->allocate();
  m_transferCmd = transferPool->allocate();
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
      allocator->get());
  m_objectBuffer->map();

  m_indirectBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(vk::DrawIndexedIndirectCommand) *
                         MAX_INDIRECT_COMMANDS,
                 .usage = vk::BufferUsageFlagBits::eIndirectBuffer |
                          vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eStorageBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get());

  // Create debug line vertex buffer
  m_debugLineVertexBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(DebugLineVertex) * 2 * MAX_LINES,
                 .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                 .memoryFlags =
                     VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT},
      allocator->get());
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
      allocator->get());

  // Allocate descriptor set with per frame descriptor set layout
  m_descriptorSet = descriptorPool->allocate(descriptorLayout->get());
}

VulkanFrame::~VulkanFrame() {
  m_objectBuffer->unmap();
  m_debugLineVertexBuffer->unmap();
}
void VulkanFrame::recreateDepthImage(const uint32_t width,
                                     const uint32_t height) {
  m_depthImage = nullptr;

  auto imageInfo = ImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eD32Sfloat,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .aspectFlags = vk::ImageAspectFlagBits::eDepth,
  };
  m_depthImage = std::make_unique<VulkanImage>(imageInfo, m_allocator->get());
}

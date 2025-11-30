#include "vk_renderer.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "render/vk_image.hpp"
#include "render/vk_instance.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/shader_resource.hpp"
#include "util/print.hpp"
#include "util/vk_transient_cmd.hpp"
#include "vk_allocator.hpp"
#include "vk_pipeline.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"

VulkanRenderer::VulkanRenderer(SDL_Window *window,
                               ResourceManager &resourceManager) {
  m_instance = std::make_unique<VulkanInstance>();

  m_surface = std::make_unique<VulkanSurface>(window, m_instance->get());

  m_device =
      std::make_unique<VulkanDevice>(m_instance->get(), m_surface->get());

  m_allocator = std::make_unique<VulkanAllocator>(
      m_device->getPhysical(), m_device->get(), m_instance->get());

  m_swapChain = std::make_unique<VulkanSwapChain>(
      m_device->get(), m_device->getPhysical(), m_surface->get());

  const uint32_t graphicsQueueFamily =
      m_device->queueFamilies().graphics.value();
  const uint32_t transferQueueFamily =
      m_device->queueFamilies().transfer.value();
  const uint32_t computeQueueFamily = m_device->queueFamilies().compute.value();

  m_graphicsPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{
          .queueFamilyIndex = graphicsQueueFamily,
          .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
      m_device->get());

  m_transferPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = transferQueueFamily, .flags = {}},
      m_device->get());

  m_computePool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = computeQueueFamily, .flags = {}},
      m_device->get());

  // create shaders, add them as stages here, etc.
  const auto vertCode = resourceManager.createFromFile<ShaderResource>(
      "../assets/shaders/test.vert.spv", ShaderResourceLoader{});
  m_vertexShader =
      std::make_unique<VulkanShader>(m_device->get(), vertCode->code);

  const auto fragCode = resourceManager.createFromFile<ShaderResource>(
      "../assets/shaders/test.frag.spv", ShaderResourceLoader{});
  m_fragmentShader =
      std::make_unique<VulkanShader>(m_device->get(), fragCode->code);
    
  const vk::PipelineShaderStageCreateInfo vertStage{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = m_vertexShader->get(),
      .pName = "main"};

  const vk::PipelineShaderStageCreateInfo fragStage{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = m_fragmentShader->get(),
      .pName = "main"};

  PipelineLayoutInfo pipelineLayoutInfo{};

  const vk::PushConstantRange pushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .offset = 0,
      .size = sizeof(PushConstant)};

  pipelineLayoutInfo.pushConstants.push_back(pushConstantRange);

  m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(m_device->get(),
                                                            pipelineLayoutInfo);

  GraphicsPipelineInfo pipelineInfo{};
  pipelineInfo.shaderStages = {vertStage, fragStage};
  pipelineInfo.colorAttachmentFormats = {m_swapChain->format()};
  pipelineInfo.layout = m_pipelineLayout->get();
  GraphicsPipelineInfo::ColorBlendAttachment colorBlend{};
  pipelineInfo.colorBlendAttachments = {colorBlend};
  pipelineInfo.frontFace = vk::FrontFace::eClockwise;

  vk::VertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = vk::VertexInputRate::eVertex;

  vk::VertexInputAttributeDescription positionAttr{};
  positionAttr.location = 0;
  positionAttr.binding = 0;
  positionAttr.format = vk::Format::eR32G32B32Sfloat;
  positionAttr.offset = 0;

  vk::VertexInputAttributeDescription colorAttr{};
  colorAttr.location = 1;
  colorAttr.binding = 0;
  colorAttr.format = vk::Format::eR32G32B32Sfloat;
  colorAttr.offset = sizeof(glm::vec3);

  pipelineInfo.vertexBindings.push_back(binding);
  pipelineInfo.vertexAttributes.push_back(positionAttr);
  pipelineInfo.vertexAttributes.push_back(colorAttr);

  m_pipeline = std::make_unique<VulkanPipeline>(m_device->get(), pipelineInfo);


  const auto compCode = resourceManager.createFromFile<ShaderResource>(
      "../assets/shaders/test.comp.spv", ShaderResourceLoader{});
  m_computeShader =
      std::make_unique<VulkanShader>(m_device->get(), compCode->code);

  const vk::PipelineShaderStageCreateInfo compStage{
      .stage = vk::ShaderStageFlagBits::eCompute,
      .module = m_computeShader->get(),
      .pName = "main"};

  const vk::PushConstantRange compPushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eCompute,
      .offset = 0,
      .size = sizeof(PushConstantCompute)};

  pipelineLayoutInfo.pushConstants.clear();
  pipelineLayoutInfo.pushConstants.push_back(compPushConstantRange);

  m_compPipelineLayout = std::make_unique<VulkanPipelineLayout>(m_device->get(),
                                                            pipelineLayoutInfo);


  ComputePipelineInfo compPipelineInfo{
    .shaderStage = compStage,
    .layout = m_compPipelineLayout->get(),
  };

  m_compPipeline = std::make_unique<VulkanPipeline>(m_device->get(), compPipelineInfo);
  
  const auto frameCount = m_swapChain->imageCount();
  m_frames.reserve(frameCount);
  for (uint32_t i = 0; i < frameCount; i++) {
    m_frames.push_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_device->get(), m_allocator.get()));

    m_frames[i]->stagingBuffer()->map();
  }

  vk::SemaphoreCreateInfo semaphoreCreateInfo{};
  vk::FenceCreateInfo fenceCreateInfo{.flags =
                                          vk::FenceCreateFlagBits::eSignaled};
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_imageAvailableSemaphores[i] =
        m_device->get().createSemaphoreUnique(semaphoreCreateInfo);
    m_inFlightFences[i] = m_device->get().createFenceUnique(fenceCreateInfo);
  }

  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_device->waitIdle();

  for (const auto &m_frame : m_frames) {
    m_frame->stagingBuffer()->unmap();
  }

  Util::println("Destroyed vulkan renderer");
}

void VulkanRenderer::run() {
  auto imageIndex = beginFrame().value_or(UINT32_MAX);

  if (imageIndex == UINT32_MAX) {
    m_swapChain->recreate();
    return;
  }

  auto &frame = m_frames[imageIndex];

  auto *stagingBuffer = frame->stagingBuffer();
  auto *indirectBuffer = frame->indirectBuffer();

  auto *mapped = stagingBuffer->getMappedDataAs<VkDrawIndexedIndirectCommand>();

  uint32_t commandCount = 0;
  for (const auto &mesh : m_meshes) {
    mapped[commandCount++] = mesh;
  }

  auto ccmd = frame->computeCmd();
  vk::CommandBufferBeginInfo beginInfo{};
  ccmd.begin(beginInfo);
    
  ccmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_compPipeline->get());
  PushConstantCompute pcComp {
    .input1 = 34.5F,
    .input2 = 34.5F
  };
  ccmd.pushConstants(m_compPipelineLayout->get(), vk::ShaderStageFlagBits::eCompute,
                    0, sizeof(PushConstantCompute), &pcComp);
  ccmd.dispatch(1, 1, 1);

  ccmd.end();

  auto cmd = frame->graphicsCmd();

  cmd.begin(beginInfo);

  vk::BufferCopy copyRegion{
      .srcOffset = 0, .dstOffset = 0, .size = stagingBuffer->size()};

  cmd.copyBuffer(stagingBuffer->get(), indirectBuffer->get(), copyRegion);

  vk::BufferMemoryBarrier2 bufferBarrier{
      .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
      .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect,
      .dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = indirectBuffer->get(),
      .offset = 0,
      .size = VK_WHOLE_SIZE};
  vk::DependencyInfo dependencyInfo{.bufferMemoryBarrierCount = 1,
                                    .pBufferMemoryBarriers = &bufferBarrier};
  cmd.pipelineBarrier2(&dependencyInfo);  

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  vk::RenderingAttachmentInfo colorAttachment{
      .imageView = m_swapChain->imageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearValue{
          .color = vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 1.0F}}}};

  vk::RenderingInfo renderInfo{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0},
                               .extent = m_swapChain->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment};

  cmd.beginRendering(renderInfo);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->get());
  
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -5.0f);
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, upVector);
  float fov = glm::radians(45.0f);   // Field of view in radians
  float aspectRatio = 16.0f / 9.0f;  // Screen aspect ratio
  float nearPlane = 0.1f;
  float farPlane = 100.0f;
  glm::mat4 projection =
      glm::perspective(fov, aspectRatio, nearPlane, farPlane);

  PushConstant pushConstant{
      .view = view,
      .proj = projection,
  };

  cmd.pushConstants(m_pipelineLayout->get(), vk::ShaderStageFlagBits::eVertex,
                    0, sizeof(PushConstant), &pushConstant);
  vk::DeviceSize offset = 0;
  vk::Buffer vertexBuffer = m_vertexBuffer->get();
  cmd.bindVertexBuffers(0, 1, &vertexBuffer, &offset);
  vk::Buffer indexBuffer = m_indexBuffer->get();
  cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

  vk::Viewport viewport{
      .x = 0.0F,
      .y = 0.0F,
      .width = static_cast<float>(m_swapChain->extent().width),
      .height = static_cast<float>(m_swapChain->extent().height),
      .minDepth = 0.0F,
      .maxDepth = 1.0F};
  cmd.setViewport(0, 1, &viewport);

  vk::Rect2D scissor{.offset = {.x = 0, .y = 0},
                     .extent = {.width = m_swapChain->extent().width,
                                .height = m_swapChain->extent().height}};
  cmd.setScissor(0, 1, &scissor);

  // --------------------------------------------------
  // MAIN RENDER LOOP
  // --------------------------------------------------
  // for (const auto &mesh : m_meshes) {
  //   cmd.drawIndexed(mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset,
  //   0);
  // }
  cmd.drawIndexedIndirect(frame->indirectBuffer()->get(), 0, m_meshes.size(),
                          sizeof(vk::DrawIndexedIndirectCommand));
  cmd.endRendering();

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

  cmd.end();

  endFrame(imageIndex);
}

void VulkanRenderer::addVertices(const std::vector<Vertex> &vertices) {
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
}

void VulkanRenderer::addIndices(const std::vector<uint32_t> &indices) {
  m_indices.insert(m_indices.end(), indices.begin(), indices.end());
}

void VulkanRenderer::addMesh(uint32_t startVertex, uint32_t startIndex,
                             uint32_t indexCount) {
  m_meshes.emplace_back(indexCount, 1, startIndex, startVertex, 0);
}

void VulkanRenderer::upload() {
  if (m_vertexBuffer) {
    m_vertexBuffer->destroy();
    m_vertexBuffer = nullptr;
  }

  if (m_indexBuffer) {
    m_indexBuffer->destroy();
    m_indexBuffer = nullptr;
  }

  auto verticesSize = sizeof(Vertex) * m_vertices.size();
  auto indicesSize = sizeof(uint32_t) * m_indices.size();

  m_vertexBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = verticesSize,
                 .usage = vk::BufferUsageFlagBits::eVertexBuffer |
                          vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      m_allocator->get());

  m_indexBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = indicesSize,
                 .usage = vk::BufferUsageFlagBits::eIndexBuffer |
                          vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      m_allocator->get());

  auto stagingVertexBuffer = VulkanBuffer(
      BufferInfo{.size = verticesSize,
                 .usage = vk::BufferUsageFlagBits::eTransferSrc,
                 .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                 .memoryFlags =
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT},
      m_allocator->get());
  stagingVertexBuffer.write(m_vertices.data());

  auto stagingIndexBuffer = VulkanBuffer(
      BufferInfo{.size = indicesSize,
                 .usage = vk::BufferUsageFlagBits::eTransferSrc,
                 .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                 .memoryFlags =
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT},
      m_allocator->get());
  stagingIndexBuffer.write(m_indices.data());

  auto cmd = Util::beginSingleTimeCommandBuffer(*m_transferPool);

  vk::BufferCopy vertexCopyRegion = {};
  vertexCopyRegion.srcOffset = 0;
  vertexCopyRegion.dstOffset = 0;
  vertexCopyRegion.size = verticesSize;
  cmd.copyBuffer(stagingVertexBuffer.get(), m_vertexBuffer->get(), 1,
                 &vertexCopyRegion);

  vk::BufferCopy indexCopyRegion = {};
  indexCopyRegion.srcOffset = 0;
  indexCopyRegion.dstOffset = 0;
  indexCopyRegion.size = indicesSize;
  cmd.copyBuffer(stagingIndexBuffer.get(), m_indexBuffer->get(), 1,
                 &indexCopyRegion);
  if (m_device->queueFamilies().transfer !=
      m_device->queueFamilies().graphics) {
    std::array<vk::BufferMemoryBarrier2, 2> releaseBarriers = {
        {{.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
          .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
          .dstStageMask = vk::PipelineStageFlagBits2::eNone,
          .dstAccessMask = vk::AccessFlagBits2::eNone,
          .srcQueueFamilyIndex = m_device->queueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = m_device->queueFamilies().graphics.value_or(0),
          .buffer = m_vertexBuffer->get(),
          .offset = 0,
          .size = vk::WholeSize},
         {.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
          .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
          .dstStageMask = vk::PipelineStageFlagBits2::eNone,
          .dstAccessMask = vk::AccessFlagBits2::eNone,
          .srcQueueFamilyIndex = m_device->queueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = m_device->queueFamilies().graphics.value_or(0),
          .buffer = m_indexBuffer->get(),
          .offset = 0,
          .size = vk::WholeSize}}};

    vk::DependencyInfoKHR dependencyInfo{
        .sType = vk::StructureType::eDependencyInfoKHR,
        .bufferMemoryBarrierCount = 2,
        .pBufferMemoryBarriers = releaseBarriers.data()};

    cmd.pipelineBarrier2(dependencyInfo);
  }
  Util::endSingleTimeCommandBuffer(cmd, m_device->transferQueue(),
                                   *m_transferPool);

  if (m_device->queueFamilies().transfer !=
      m_device->queueFamilies().graphics) {
    auto graphicsCmd = Util::beginSingleTimeCommandBuffer(*m_graphicsPool);

    std::array<vk::BufferMemoryBarrier2, 2> acquireBarriers = {
        {{.srcStageMask = vk::PipelineStageFlagBits2::eNone,
          .srcAccessMask = vk::AccessFlagBits2::eNone,
          .dstStageMask = vk::PipelineStageFlagBits2::eVertexInput,
          .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
          .srcQueueFamilyIndex = m_device->queueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = m_device->queueFamilies().graphics.value_or(0),
          .buffer = m_vertexBuffer->get(),
          .offset = 0,
          .size = vk::WholeSize},
         {.srcStageMask = vk::PipelineStageFlagBits2::eNone,
          .srcAccessMask = vk::AccessFlagBits2::eNone,
          .dstStageMask = vk::PipelineStageFlagBits2::eIndexInput,
          .dstAccessMask = vk::AccessFlagBits2::eIndexRead,
          .srcQueueFamilyIndex = m_device->queueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = m_device->queueFamilies().graphics.value_or(0),
          .buffer = m_indexBuffer->get(),
          .offset = 0,
          .size = vk::WholeSize}}};

    vk::DependencyInfo acquireDependencyInfo{
        .bufferMemoryBarrierCount = 2,
        .pBufferMemoryBarriers = acquireBarriers.data()};

    graphicsCmd.pipelineBarrier2(acquireDependencyInfo);

    Util::endSingleTimeCommandBuffer(graphicsCmd, m_device->graphicsQueue(),
                                     *m_graphicsPool);
  }
}

uint32_t VulkanRenderer::getVertexCount() const { return m_vertices.size(); }
uint32_t VulkanRenderer::getIndexCount() const { return m_indices.size(); }

std::optional<uint32_t> VulkanRenderer::beginFrame() {
  auto result = m_device->get().waitForFences(*m_inFlightFences[m_currentFrame],
                                              VK_TRUE, UINT64_MAX);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("waitForFences failed: " + vk::to_string(result));
  }

  m_device->get().resetFences(*m_inFlightFences[m_currentFrame]);

  uint32_t imageIndex{};
  result = m_swapChain->acquireNextImage(
      *m_imageAvailableSemaphores[m_currentFrame], imageIndex);

  if (result == vk::Result::eErrorOutOfDateKHR ||
      result == vk::Result::eSuboptimalKHR) {
    return std::nullopt;
  }

  return imageIndex;
}

void VulkanRenderer::endFrame(uint32_t imageIndex) {
  auto &frame = m_frames[imageIndex];
  auto cmd = frame->graphicsCmd();

  vk::SemaphoreSubmitInfo waitSemaphore{
      .semaphore = *m_imageAvailableSemaphores[m_currentFrame],
      .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput};

  vk::SemaphoreSubmitInfo signalSemaphore{
      .semaphore = frame->renderFinished(),
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands};

  vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd};

  vk::SubmitInfo2 submitInfo{.waitSemaphoreInfoCount = 1,
                             .pWaitSemaphoreInfos = &waitSemaphore,
                             .commandBufferInfoCount = 1,
                             .pCommandBufferInfos = &cmdInfo,
                             .signalSemaphoreInfoCount = 1,
                             .pSignalSemaphoreInfos = &signalSemaphore};

  auto result = m_device->graphicsQueue().submit2(
      1, &submitInfo, *m_inFlightFences[m_currentFrame]);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("submit2 failed: " + vk::to_string(result));
  }

  result = m_swapChain->present(imageIndex, m_device->presentQueue(),
                                frame->renderFinished());

  if (result == vk::Result::eErrorOutOfDateKHR ||
      result == vk::Result::eSuboptimalKHR) {
    m_swapChain->recreate();
    return;
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

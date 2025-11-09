#include "vk_renderer.hpp"

#include <SDL3/SDL.h>

#include <memory>

#include "render/vk_image.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/shader_resource.hpp"
#include "util/print.hpp"
#include "vk_allocator.hpp"
#include "vk_pipeline.hpp"

VulkanRenderer::VulkanRenderer(SDL_Window* window,
                               ResourceManager& resourceManager) {
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

  const auto frameCount = m_swapChain->imageCount();
  m_frames.reserve(frameCount);
  for (uint32_t i = 0; i < frameCount; ++i) {
    m_frames.push_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_device->get()));
  }

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

  m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(m_device->get(),
                                                            pipelineLayoutInfo);

  // create shaders, add them as stages here, etc.
  PipelineInfo pipelineInfo{};
  pipelineInfo.shaderStages = {vertStage, fragStage};
  pipelineInfo.colorAttachmentFormats = {m_swapChain->format()};
  pipelineInfo.layout = m_pipelineLayout->get();
  PipelineInfo::ColorBlendAttachment colorBlend{};
  pipelineInfo.colorBlendAttachments = {colorBlend};
  m_pipeline = std::make_unique<VulkanPipeline>(
      m_device->get(), VulkanPipelineType::Graphics, pipelineInfo);

  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_device->waitIdle();
  Util::println("Destroyed vulkan renderer");
}

void VulkanRenderer::run() {
  auto& frame = m_frames[m_currentFrame];

  auto result =
      m_device->get().waitForFences(frame->inFlight(), VK_TRUE, UINT64_MAX);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("waitForFences failed: " + vk::to_string(result));
  }

  uint32_t imageIndex{};
  result = m_swapChain->acquireNextImage(frame->imageAvailable(), imageIndex);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    m_swapChain->recreate();
    return;
  }

  m_device->get().resetFences(frame->inFlight());

  auto cmd = frame->graphicsCmd();

  vk::CommandBufferBeginInfo beginInfo{};
  cmd.begin(beginInfo);

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
      .renderArea =
          vk::Rect2D{.offset = {0, 0}, .extent = m_swapChain->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment};

  cmd.beginRendering(renderInfo);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->get());

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

  cmd.draw(3, 1, 0, 0);
  cmd.endRendering();

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

  cmd.end();

  vk::SemaphoreSubmitInfo waitSemaphore{
      .semaphore = frame->imageAvailable(),
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

  result = m_device->graphicsQueue().submit2(1, &submitInfo, frame->inFlight());
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("submit2 failed: " + vk::to_string(result));
  }

  m_swapChain->present(imageIndex, m_device->presentQueue(),
                       frame->renderFinished());

  m_currentFrame = (m_currentFrame + 1) % m_frames.size();
}
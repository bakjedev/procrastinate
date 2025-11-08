#include "render/vk_renderer.hpp"

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

  m_physicalDevice = std::make_unique<VulkanPhysicalDevice>(m_instance->get(),
                                                            m_surface->get());

  m_logicalDevice = std::make_unique<VulkanLogicalDevice>(
      m_physicalDevice->get(), m_physicalDevice->features(),
      m_physicalDevice->queueFamilies(), m_physicalDevice->features12(),
      m_physicalDevice->features13());

  m_allocator = std::make_unique<VulkanAllocator>(
      m_physicalDevice->get(), m_logicalDevice->get(), m_instance->get());

  m_swapChain = std::make_unique<VulkanSwapChain>(
      m_logicalDevice->get(), m_physicalDevice->get(), m_surface->get());

  const uint32_t graphicsQueueFamily =
      m_physicalDevice->queueFamilies().graphics.value();
  const uint32_t transferQueueFamily =
      m_physicalDevice->queueFamilies().transfer.value();
  const uint32_t computeQueueFamily =
      m_physicalDevice->queueFamilies().compute.value();

  m_graphicsPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = graphicsQueueFamily,
                      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT},
      m_logicalDevice->get());

  m_transferPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = transferQueueFamily},
      m_logicalDevice->get());

  m_computePool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = computeQueueFamily},
      m_logicalDevice->get());

  const auto frameCount = m_swapChain->imageCount();
  m_frames.reserve(frameCount);
  for (uint32_t i = 0; i < frameCount; ++i) {
    m_frames.push_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_logicalDevice->get()));
  }

  const auto vertCode = resourceManager.createFromFile<ShaderResource>(
      "../assets/shaders/test.vert.spv", ShaderResourceLoader{});
  m_vertexShader =
      std::make_unique<VulkanShader>(m_logicalDevice->get(), vertCode->code);

  const auto fragCode = resourceManager.createFromFile<ShaderResource>(
      "../assets/shaders/test.frag.spv", ShaderResourceLoader{});
  m_fragmentShader =
      std::make_unique<VulkanShader>(m_logicalDevice->get(), fragCode->code);

  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = m_vertexShader->get();
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = m_fragmentShader->get();
  fragStage.pName = "main";

  PipelineLayoutInfo pipelineLayoutInfo{};

  m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(
      m_logicalDevice->get(), pipelineLayoutInfo);

  // create shaders, add them as stages here, etc.
  PipelineInfo pipelineInfo{};
  pipelineInfo.shaderStages = {vertStage, fragStage};
  pipelineInfo.colorAttachmentFormats = {m_swapChain->format()};
  pipelineInfo.layout = m_pipelineLayout->get();
  PipelineInfo::ColorBlendAttachment colorBlend{};
  pipelineInfo.colorBlendAttachments = {colorBlend};
  m_pipeline = std::make_unique<VulkanPipeline>(
      m_logicalDevice->get(), VulkanPipelineType::Graphics, pipelineInfo);

  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_logicalDevice->waitIdle();
  Util::println("Destroyed vulkan renderer");
}

void VulkanRenderer::run() {
  auto& frame = m_frames[m_currentFrame];
  vkWaitForFences(m_logicalDevice->get(), 1, &frame->inFlight(), VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex{};
  auto result =
      m_swapChain->acquireNextImage(frame->imageAvailable(), imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    m_swapChain->recreate();
    return;
  }

  vkResetFences(m_logicalDevice->get(), 1, &frame->inFlight());

  auto* cmd = frame->graphics();
  // vkResetCommandBuffer(cmd, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(cmd, &beginInfo);

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = m_swapChain->imageViews()[imageIndex];
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue.color = {{0.0F, 0.0F, 0.0F, 1.0F}};

  VkRenderingInfo renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea.extent = m_swapChain->extent();
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(cmd, &renderInfo);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->get());

  VkViewport viewport{};
  viewport.width = static_cast<float>(m_swapChain->extent().width);
  viewport.height = static_cast<float>(m_swapChain->extent().height);
  viewport.maxDepth = 1.0F;
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = {.width = m_swapChain->extent().width,
                    .height = m_swapChain->extent().height};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdDraw(cmd, 3, 1, 0, 0);
  vkCmdEndRendering(cmd);

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  vkEndCommandBuffer(cmd);

  VkSemaphoreSubmitInfo waitSemaphore{};
  waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitSemaphore.semaphore = frame->imageAvailable();
  waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSemaphoreSubmitInfo signalSemaphore{};
  signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalSemaphore.semaphore = frame->renderFinished();
  signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

  VkCommandBufferSubmitInfo cmdInfo{};
  cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdInfo.commandBuffer = cmd;

  VkSubmitInfo2 submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.waitSemaphoreInfoCount = 1;
  submitInfo.pWaitSemaphoreInfos = &waitSemaphore;
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = &cmdInfo;
  submitInfo.signalSemaphoreInfoCount = 1;
  submitInfo.pSignalSemaphoreInfos = &signalSemaphore;

  vkQueueSubmit2(m_logicalDevice->graphicsQueue(), 1, &submitInfo,
                 frame->inFlight());

  m_swapChain->present(imageIndex, m_logicalDevice->presentQueue(),
                       frame->renderFinished());

  m_currentFrame = (m_currentFrame + 1) % m_frames.size();
}
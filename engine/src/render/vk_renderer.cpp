#include "vk_renderer.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <array>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "render/vk_descriptor.hpp"
#include "render/vk_image.hpp"
#include "core/window.hpp"
#include "render/vk_instance.hpp"
#include "resource/resource_manager.hpp"
#include "core/events.hpp"
#include "resource/types/shader_resource.hpp"
#include "util/print.hpp"
#include "util/vk_transient_cmd.hpp"
#include "vk_allocator.hpp"
#include "vk_pipeline.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl3.h"

VulkanRenderer::VulkanRenderer(Window *window,
                               ResourceManager &resourceManager, EventManager& eventManager) {
  // -----------------------------------------------------------
  // BASIC VULKAN OBJECTS
  // -----------------------------------------------------------
  m_instance = std::make_unique<VulkanInstance>();

  m_surface = std::make_unique<VulkanSurface>(window->get(), m_instance->get());

  m_device =
      std::make_unique<VulkanDevice>(m_instance->get(), m_surface->get());

  m_allocator = std::make_unique<VulkanAllocator>(
      m_device->getPhysical(), m_device->get(), m_instance->get());

  // -----------------------------------------------------------
  // SWAPCHAIN
  // -----------------------------------------------------------
  auto extent = window->getWindowSize();
  m_swapChain = std::make_unique<VulkanSwapChain>(
      m_device->get(), m_device->getPhysical(), m_surface->get(), vk::Extent2D{.width = extent.first, .height = extent.second});

  // -----------------------------------------------------------
  // GET QUEUE FAMILIES FOR GRAPHICS, TRANSFER AND COMPUTE
  // -----------------------------------------------------------
  const uint32_t graphicsQueueFamily =
      m_device->queueFamilies().graphics.value();
  const uint32_t transferQueueFamily =
      m_device->queueFamilies().transfer.value();
  const uint32_t computeQueueFamily = m_device->queueFamilies().compute.value();

  // -----------------------------------------------------------
  // CREATE COMMAND POOLS FOR GRAPHICS, TRANSFER AND COMPUTE
  // -----------------------------------------------------------
  m_graphicsPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{
          .queueFamilyIndex = graphicsQueueFamily,
          .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
      m_device->get());

  m_transferPool = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queueFamilyIndex = transferQueueFamily, .flags = {}},
      m_device->get());

  m_computePool = std::make_unique<VulkanCommandPool>(
    CommandPoolInfo{
      .queueFamilyIndex = computeQueueFamily,
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    },
    m_device->get()
  );

  // -----------------------------------------------------------
  // LOAD AND CREATE GRAPHICS SHADERS
  // -----------------------------------------------------------
  const auto vertCode = resourceManager.createFromFile<ShaderResource>(
      "engine/assets/shaders/test.vert.spv", ShaderResourceLoader{});
  m_vertexShader =
      std::make_unique<VulkanShader>(m_device->get(), vertCode->code);

  const auto fragCode = resourceManager.createFromFile<ShaderResource>(
      "engine/assets/shaders/test.frag.spv", ShaderResourceLoader{});
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


  // -----------------------------------------------------------
  // Descriptor Set Layouts
  // -----------------------------------------------------------
  m_frameDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(
    m_device->get(),
    std::vector{
      vk::DescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      },
      vk::DescriptorSetLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex
      }
    },
    std::vector<vk::DescriptorBindingFlags>{},
    vk::DescriptorSetLayoutCreateFlags{}
  );

  m_staticDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(
    m_device->get(),
    std::vector{
      vk::DescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      }
    },
    std::vector<vk::DescriptorBindingFlags>{},
    vk::DescriptorSetLayoutCreateFlags{}
  );

  // -----------------------------------------------------------
  // CREATE GRAPHICS PIPELINE LAYOUT
  // -----------------------------------------------------------
  PipelineLayoutInfo pipelineLayoutInfo{};

  const vk::PushConstantRange pushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .offset = 0,
      .size = sizeof(PushConstant)};

  pipelineLayoutInfo.pushConstants.push_back(pushConstantRange);
  pipelineLayoutInfo.descriptorSets.push_back(m_frameDescriptorSetLayout->get());

  m_pipelineLayout = std::make_unique<VulkanPipelineLayout>(m_device->get(),
                                                            pipelineLayoutInfo);

  // -----------------------------------------------------------
  // CREATE GRAPHICS PIPELINE
  // -----------------------------------------------------------
  GraphicsPipelineInfo pipelineInfo{};
  pipelineInfo.shaderStages = {vertStage, fragStage};
  pipelineInfo.colorAttachmentFormats = {m_swapChain->format()};
  pipelineInfo.layout = m_pipelineLayout->get();
  constexpr GraphicsPipelineInfo::ColorBlendAttachment colorBlend{};
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


  Util::println("Creating Graphics pipeline");
  m_pipeline = std::make_unique<VulkanPipeline>(m_device->get(), pipelineInfo);

  // -----------------------------------------------------------
  // LOAD AND CREATE COMPUTE SHADER
  // -----------------------------------------------------------
  const auto compCode = resourceManager.createFromFile<ShaderResource>(
      "engine/assets/shaders/test.comp.spv", ShaderResourceLoader{});
  m_computeShader =
      std::make_unique<VulkanShader>(m_device->get(), compCode->code);

  const vk::PipelineShaderStageCreateInfo compStage{
      .stage = vk::ShaderStageFlagBits::eCompute,
      .module = m_computeShader->get(),
      .pName = "main"};

  // -----------------------------------------------------------
  // CREATE COMPUTE PIPELINE LAYOUT (DESCRIPTOR SET LAYOUT)
  // -----------------------------------------------------------
  pipelineLayoutInfo.pushConstants.clear();
  pipelineLayoutInfo.descriptorSets.clear();

  pipelineLayoutInfo.descriptorSets.push_back(m_staticDescriptorSetLayout->get());
  pipelineLayoutInfo.descriptorSets.push_back(m_frameDescriptorSetLayout->get());

  m_compPipelineLayout = std::make_unique<VulkanPipelineLayout>(m_device->get(),
                                                            pipelineLayoutInfo);
  // -----------------------------------------------------------
  // CREATE COMPUTE PIPELINE
  // -----------------------------------------------------------
  ComputePipelineInfo compPipelineInfo{
    .shaderStage = compStage,
    .layout = m_compPipelineLayout->get(),
  };

  Util::println("Creating Compute pipeline");
  m_compPipeline = std::make_unique<VulkanPipeline>(m_device->get(), compPipelineInfo);

  // -----------------------------------------------------------
  // CREATE DESCRIPTOR POOL
  // -----------------------------------------------------------
  DescriptorPoolInfo descriptorPoolInfo{};
  descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  descriptorPoolInfo.maxSets = 1000;
  descriptorPoolInfo.poolSizes = {
    {.type=vk::DescriptorType::eCombinedImageSampler, .descriptorCount=1},
    {.type=vk::DescriptorType::eStorageBuffer, .descriptorCount=20}
  };

  m_descriptorPool = std::make_unique<VulkanDescriptorPool>(m_device->get(), descriptorPoolInfo);

  // -----------------------------------------------------------
  // CREATE FRAME RESOURCES
  // -----------------------------------------------------------
  m_frames.reserve(MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_frames.push_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_descriptorPool.get(), m_frameDescriptorSetLayout.get(), m_device->get(), m_allocator.get()));
  }
  // -----------------------------------------------------------
  // ALLOCATE STATIC DESCRIPTOR SET
  // -----------------------------------------------------------

  m_staticDescriptorSet = m_descriptorPool->allocate(m_staticDescriptorSetLayout->get());

  // -----------------------------------------------------------
  // WRITE TO DESCRIPTOR SETS
  // -----------------------------------------------------------
  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(m_frames.size() * 2);
  std::vector<vk::DescriptorBufferInfo> bufferInfos;
  bufferInfos.reserve(m_frames.size() * 2);

  for(const auto& frame : m_frames) {
    bufferInfos.push_back({
      .buffer = frame->indirectBuffer()->get(),
      .offset = 0,
      .range = vk::WholeSize
    });
    
    const vk::WriteDescriptorSet write{
      .dstSet = frame->descriptorSet(),
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &bufferInfos.back()
    };

    writes.push_back(write);

    bufferInfos.push_back({
      .buffer = frame->objectBuffer()->get(),
      .offset = 0,
      .range = vk::WholeSize
    });
    
    const vk::WriteDescriptorSet write2{
      .dstSet = frame->descriptorSet(),
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .pBufferInfo = &bufferInfos.back()
    };

    writes.push_back(write2);
  }

  m_device->get().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

  // -----------------------------------------------------------
  // CREATE FRAME SYNC OBJECTS
  // -----------------------------------------------------------
  constexpr vk::FenceCreateInfo fenceCreateInfo{.flags =
                                          vk::FenceCreateFlagBits::eSignaled};
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    m_imageAvailableSemaphores[i] =
        m_device->get().createSemaphoreUnique(semaphoreCreateInfo);
    m_inFlightFences[i] = m_device->get().createFenceUnique(fenceCreateInfo);
  }

  // -----------------------------------------------------------
  // SET REFERENCES
  // -----------------------------------------------------------
  m_window = window;
  m_eventManager = &eventManager;

  // -----------------------------------------------------------
  // INITIALIZE ImGui
  // -----------------------------------------------------------
  auto format = m_swapChain->format();
  vk::PipelineRenderingCreateInfo renderingInfo{
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &format
  };

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = m_instance->get();
  initInfo.PhysicalDevice = m_device->getPhysical();
  initInfo.Device = m_device->get();
  initInfo.QueueFamily = graphicsQueueFamily;
  initInfo.Queue = m_device->graphicsQueue();
  initInfo.DescriptorPool = m_descriptorPool->get();
  initInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = m_frames.size();
  initInfo.UseDynamicRendering = true;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingInfo;

  ImGui_ImplVulkan_Init(&initInfo);

  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_device->waitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  Util::println("Destroyed vulkan renderer");
}

void VulkanRenderer::run() {
  for (const auto& event : m_eventManager->getEvents()) {
    switch (event.type) {
      case EventType::WindowResized: {
        const auto& windowResize = std::get<WindowResizeData>(event.data);
        Util::println("Got a window resize event");
        m_swapChain->recreate({.width = windowResize.width, .height = windowResize.height});
        break;
      }
      default:
        break;
    }
  }
    
  auto imageIndex = beginFrame().value_or(UINT32_MAX);

  if (imageIndex == UINT32_MAX) {
    auto extent = m_window->getWindowSize();
    Util::println("Failed to get the image index");
    m_swapChain->recreate({.width=extent.first, .height=extent.second});
    return;
  }

  auto &frame = m_frames[m_currentFrame];

  // if (!m_renderObjects.empty()) {
  //   frame->objectBuffer()->writeRange(m_renderObjects.data(), sizeof(RenderObject) * m_renderObjects.size());
  // }
  size_t writeOffset = 0;
  for (const auto& renderObject : m_renderObjects) {
    constexpr auto size = sizeof(RenderObject);
    frame->objectBuffer()->writeRangeOffset(&renderObject, size, writeOffset);
    writeOffset += size;
  }

  // auto tempVector = std::vector<RenderObject>{};
  // tempVector.reserve(m_renderObjects.size());
  // for (const auto& id : m_renderObjects) {
  //   tempVector.push_back(m_objects.at(id));
  // }
  // frame->objectBuffer()->writeRange(tempVector.data(), sizeof(RenderObject) * tempVector.size());



  auto ccmd = frame->computeCmd();
  constexpr vk::CommandBufferBeginInfo beginInfo{};
  ccmd.begin(beginInfo);

  auto descriptorSets = std::array{m_staticDescriptorSet, frame->descriptorSet()};

  ccmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                          m_compPipelineLayout->get(),
                          0, descriptorSets.size(),
                          descriptorSets.data(),
                          0, nullptr);

  ccmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_compPipeline->get());
  const uint32_t workgroups = (m_renderObjects.size() + 255) / 256;
  ccmd.dispatch(workgroups, 1, 1);

  ccmd.end();

  auto cmd = frame->graphicsCmd();

  cmd.begin(beginInfo);

  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  const vk::RenderingAttachmentInfo colorAttachment{
      .imageView = m_swapChain->imageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearValue{
          .color = vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 1.0F}}}};

  const vk::RenderingInfo renderInfo{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0},
                               .extent = m_swapChain->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment};

  cmd.beginRendering(renderInfo);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->get());
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                          m_pipelineLayout->get(),
                          0, 1,
                          &frame->descriptorSet(),
                          0, nullptr);

  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -5.0f);
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, upVector);
  float fov = glm::radians(45.0f);   // Field of view in radians
  float aspectRatio = static_cast<float>(m_window->getWindowSize().first) / static_cast<float>(m_window->getWindowSize().second);  // Screen aspect ratio
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
  cmd.drawIndexedIndirect(frame->indirectBuffer()->get(), 0, m_renderObjects.size(),
                          sizeof(vk::DrawIndexedIndirectCommand));
  cmd.endRendering();

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("uhh");
  ImGui::End();

  ImGui::Render();

  vk::RenderingAttachmentInfo imguiColorAttachment{
      .imageView = m_swapChain->imageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,  // Load existing scene
      .storeOp = vk::AttachmentStoreOp::eStore,
  };

  vk::RenderingInfo imguiRenderInfo{
      .renderArea = vk::Rect2D{.offset = {.x=0, .y=0}, .extent = m_swapChain->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &imguiColorAttachment
  };

  cmd.beginRendering(imguiRenderInfo);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  cmd.endRendering();  
  VulkanImage::transitionImageLayout(m_swapChain->getImage(imageIndex), cmd,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

  cmd.end();

  endFrame(imageIndex);
}

uint32_t VulkanRenderer::addMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t firstIndex, int32_t vertexOffset) {
  const uint32_t meshID = m_meshInfos.size();
  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
  m_indices.insert(m_indices.end(), indices.begin(), indices.end());
  m_meshInfos.emplace_back(indices.size(), firstIndex, vertexOffset);
  return meshID;
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

  if (m_meshInfoBuffer) {
    m_meshInfoBuffer->destroy();
    m_meshInfoBuffer = nullptr;
  }

  const auto verticesSize = sizeof(Vertex) * m_vertices.size();
  const auto indicesSize = sizeof(uint32_t) * m_indices.size();
  const auto meshesSize = sizeof(MeshInfo) * m_meshInfos.size();

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

  m_meshInfoBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = meshesSize,
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer |
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

  auto stagingMeshInfoBuffer = VulkanBuffer(
    BufferInfo{.size = meshesSize,
               .usage = vk::BufferUsageFlagBits::eTransferSrc,
               .memoryUsage = VMA_MEMORY_USAGE_AUTO,
               .memoryFlags =
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                   VMA_ALLOCATION_CREATE_MAPPED_BIT},
    m_allocator->get());
  stagingMeshInfoBuffer.write(m_meshInfos.data());

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

  vk::BufferCopy meshInfoCopyRegion = {};
  meshInfoCopyRegion.srcOffset = 0;
  meshInfoCopyRegion.dstOffset = 0;
  meshInfoCopyRegion.size = meshesSize;
  cmd.copyBuffer(stagingMeshInfoBuffer.get(), m_meshInfoBuffer->get(), 1,
                 &meshInfoCopyRegion);

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

    const vk::DependencyInfoKHR dependencyInfo{
        .sType = vk::StructureType::eDependencyInfoKHR,
        .bufferMemoryBarrierCount = 2,
        .pBufferMemoryBarriers = releaseBarriers.data()};

    cmd.pipelineBarrier2(dependencyInfo);
  }
  Util::endSingleTimeCommandBuffer(cmd, m_device->transferQueue(),
                                   *m_transferPool);

  // UPDATE MESH INFO DESCRIPTOR SET
  auto bufferInfo = vk::DescriptorBufferInfo{
      .buffer = m_meshInfoBuffer->get(),
      .offset = 0,
      .range = vk::WholeSize
  };

  const vk::WriteDescriptorSet write{
    .dstSet = m_staticDescriptorSet,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = vk::DescriptorType::eStorageBuffer,
    .pBufferInfo = &bufferInfo
  };
  m_device->get().updateDescriptorSets(1, &write, 0, nullptr);

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

    const vk::DependencyInfo acquireDependencyInfo{
        .bufferMemoryBarrierCount = 2,
        .pBufferMemoryBarriers = acquireBarriers.data()};

    graphicsCmd.pipelineBarrier2(acquireDependencyInfo);

    Util::endSingleTimeCommandBuffer(graphicsCmd, m_device->graphicsQueue(),
                                     *m_graphicsPool);
  }
}

void VulkanRenderer::renderMesh(glm::mat4 model, const uint32_t meshID) {
  m_renderObjects.emplace_back(model, meshID);
}

void VulkanRenderer::clearMeshes() {
  m_renderObjects.clear();
}

int32_t VulkanRenderer::getVertexCount() const { return m_vertices.size(); }
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
  auto &frame = m_frames[m_currentFrame];

  vk::SemaphoreSubmitInfo cSignalSemaphore{
    .semaphore = frame->computeFinished(),
    .stageMask = vk::PipelineStageFlagBits2::eAllCommands
  };

  vk::CommandBufferSubmitInfo cCmdInfo {
    .commandBuffer = frame->computeCmd()
  };

  vk::SubmitInfo2 cSubmitInfo{
    .waitSemaphoreInfoCount = 0,
    .pWaitSemaphoreInfos = nullptr,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = & cCmdInfo,
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos = &cSignalSemaphore
  };

  auto result = m_device->computeQueue().submit2(1, &cSubmitInfo, nullptr);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("compute submit2 failed: " + vk::to_string(result));
  }

  std::array<vk::SemaphoreSubmitInfo, 2> gWaitSemaphores {{
    {
      .semaphore = frame->computeFinished(),
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    },
    {
      .semaphore = *m_imageAvailableSemaphores[m_currentFrame],
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    }
  }};

  vk::SemaphoreSubmitInfo gSignalSemaphore{
      .semaphore = frame->renderFinished(),
      .stageMask = vk::PipelineStageFlagBits2::eAllCommands};

  vk::CommandBufferSubmitInfo gCmdInfo{
    .commandBuffer = frame->graphicsCmd()
  };
 
  vk::SubmitInfo2 gSubmitInfo{.waitSemaphoreInfoCount = gWaitSemaphores.size(),
                             .pWaitSemaphoreInfos = gWaitSemaphores.data(),
                             .commandBufferInfoCount = 1,
                             .pCommandBufferInfos = &gCmdInfo,
                             .signalSemaphoreInfoCount = 1,
                             .pSignalSemaphoreInfos = &gSignalSemaphore};

  result = m_device->graphicsQueue().submit2(
      1, &gSubmitInfo, *m_inFlightFences[m_currentFrame]);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("graphics submit2 failed: " + vk::to_string(result));
  }

  result = m_swapChain->present(imageIndex, m_device->presentQueue(),
                                frame->renderFinished());

  if (result == vk::Result::eErrorOutOfDateKHR ||
      result == vk::Result::eSuboptimalKHR) {
    auto extent = m_window->getWindowSize();
    Util::println("Got an error in endframe");
    m_swapChain->recreate({.width = extent.first, .height = extent.second});
    return;
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

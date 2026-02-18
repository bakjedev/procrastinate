#include "vk_renderer.hpp"

#include <SDL3/SDL.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "core/events.hpp"
#include "core/window.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "render/vk_barriers.hpp"
#include "render/vk_buffer.hpp"
#include "render/vk_descriptor.hpp"
#include "render/vk_device.hpp"
#include "render/vk_frame.hpp"
#include "render/vk_image.hpp"
#include "render/vk_instance.hpp"
#include "render/vk_shader.hpp"
#include "render/vk_surface.hpp"
#include "render/vk_swap_chain.hpp"
#include "resource/resource_manager.hpp"
#include "resource/types/shader_resource.hpp"
#include "tracy/Tracy.hpp"
#include "util/print.hpp"
#include "util/vk_transient_cmd.hpp"
#include "vk_allocator.hpp"
#include "vk_pipeline.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"

constexpr float NEAR_PLANE_DISTANCE = 0.01F;
constexpr float FAR_PLANE_DISTANCE = 1000.0F;
constexpr uint32_t MAX_DESCRIPTOR_SETS = 1000;
constexpr uint32_t STORAGE_BUFFER_COUNT = 20;

VulkanRenderer::VulkanRenderer(Window* window, ResourceManager& resource_manager, EventManager& event_manager)
{
  util::println("Initializing renderer");
  // -----------------------------------------------------------
  // SET REFERENCES
  // -----------------------------------------------------------
  window_ = window;
  event_manager_ = &event_manager;

  const auto [width, height] = window->GetWindowSize();
  aspect_ratio_ = static_cast<float>(width) / static_cast<float>(height);

  // -----------------------------------------------------------
  // BASIC VULKAN OBJECTS
  // -----------------------------------------------------------
  instance_ = std::make_unique<VulkanInstance>();

  surface_ = std::make_unique<VulkanSurface>(window->get(), instance_->get());

  device_ = std::make_unique<VulkanDevice>(instance_->get(), surface_->get());

  allocator_ = std::make_unique<VulkanAllocator>(device_->GetPhysical(), device_->get(), instance_->get());

  // -----------------------------------------------------------
  // SWAPCHAIN
  // -----------------------------------------------------------
  swap_chain_ = std::make_unique<VulkanSwapChain>(device_->get(), device_->GetPhysical(), surface_->get(),
                                                  vk::Extent2D{.width = width, .height = height});
  // -----------------------------------------------------------
  // GET QUEUE FAMILIES FOR GRAPHICS, TRANSFER AND COMPUTE
  // -----------------------------------------------------------
  const uint32_t graphicsQueueFamily = device_->QueueFamilies().graphics.value();
  const uint32_t transferQueueFamily = device_->QueueFamilies().transfer.value();
  const uint32_t computeQueueFamily = device_->QueueFamilies().compute.value();

  // -----------------------------------------------------------
  // CREATE COMMAND POOLS FOR GRAPHICS, TRANSFER AND COMPUTE
  // -----------------------------------------------------------
  graphics_pool_ =
      std::make_unique<VulkanCommandPool>(CommandPoolInfo{.queue_family_index = graphicsQueueFamily,
                                                          .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
                                          device_->get());

  transfer_pool_ = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queue_family_index = transferQueueFamily, .flags = {}}, device_->get());

  compute_pool_ = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{
          .queue_family_index = computeQueueFamily,
          .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      },
      device_->get());

  // -----------------------------------------------------------
  // TRANSITION SWAPCHAIN IMAGES TO PRESENT
  // -----------------------------------------------------------
  {
    auto cmd = util::BeginSingleTimeCommandBuffer(*graphics_pool_);
    for (const auto& image: swap_chain_->images())
    {
      VulkanImage::TransitionImageLayout(image, cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
    }
    util::EndSingleTimeCommandBuffer(cmd, device_->GraphicsQueue(), *graphics_pool_);
  }
  // -----------------------------------------------------------
  // LOAD AND CREATE GRAPHICS SHADERS
  // -----------------------------------------------------------
  const auto vertCode =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.vert.spv", ShaderResourceLoader{});
  vertex_shader_ = std::make_unique<VulkanShader>(device_->get(), vertCode->code);

  const auto fragCode =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.frag.spv", ShaderResourceLoader{});
  fragment_shader_ = std::make_unique<VulkanShader>(device_->get(), fragCode->code);

  const vk::PipelineShaderStageCreateInfo vertStage{
      .stage = vk::ShaderStageFlagBits::eVertex, .module = vertex_shader_->get(), .pName = "main"};

  const vk::PipelineShaderStageCreateInfo fragStage{
      .stage = vk::ShaderStageFlagBits::eFragment, .module = fragment_shader_->get(), .pName = "main"};

  const auto debugLineVertCode = resource_manager.createFromFile<ShaderResource>(
      "engine/assets/shaders/debug_line.vert.spv", ShaderResourceLoader{});
  debug_line_vert_ = std::make_unique<VulkanShader>(device_->get(), debugLineVertCode->code);

  const auto debugLineFragCode = resource_manager.createFromFile<ShaderResource>(
      "engine/assets/shaders/debug_line.frag.spv", ShaderResourceLoader{});
  debug_line_frag_ = std::make_unique<VulkanShader>(device_->get(), debugLineFragCode->code);

  const vk::PipelineShaderStageCreateInfo debugLineVertStage{
      .stage = vk::ShaderStageFlagBits::eVertex, .module = debug_line_vert_->get(), .pName = "main"};

  const vk::PipelineShaderStageCreateInfo debugLineFragStage{
      .stage = vk::ShaderStageFlagBits::eFragment, .module = debug_line_frag_->get(), .pName = "main"};

  // -----------------------------------------------------------
  // Descriptor Set Layouts
  // -----------------------------------------------------------
  frame_descriptor_set_layout_ = std::make_unique<VulkanDescriptorSetLayout>(
      device_->get(),
      std::vector{vk::DescriptorSetLayoutBinding{// outputCommands
                                                 .binding = 0,
                                                 .descriptorType = vk::DescriptorType::eStorageBuffer,
                                                 .descriptorCount = 1,
                                                 .stageFlags = vk::ShaderStageFlagBits::eCompute},
                  vk::DescriptorSetLayoutBinding{
                      // renderObjects
                      .binding = 1,
                      .descriptorType = vk::DescriptorType::eStorageBuffer,
                      .descriptorCount = 1,
                      .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex},
                  vk::DescriptorSetLayoutBinding{// drawCount
                                                 .binding = 2,
                                                 .descriptorType = vk::DescriptorType::eStorageBuffer,
                                                 .descriptorCount = 1,
                                                 .stageFlags = vk::ShaderStageFlagBits::eCompute}},
      std::vector<vk::DescriptorBindingFlags>{}, vk::DescriptorSetLayoutCreateFlags{});

  static_descriptor_set_layout_ = std::make_unique<VulkanDescriptorSetLayout>(
      device_->get(),
      std::vector{
          vk::DescriptorSetLayoutBinding{
              // Mesh infos
              .binding = 0,
              .descriptorType = vk::DescriptorType::eStorageBuffer,
              .descriptorCount = 1,
              .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eVertex},
      },
      std::vector<vk::DescriptorBindingFlags>{}, vk::DescriptorSetLayoutCreateFlags{});

  // -----------------------------------------------------------
  // CREATE GRAPHICS PIPELINE LAYOUTS
  // -----------------------------------------------------------
  PipelineLayoutInfo pipelineLayoutInfo{};

  constexpr vk::PushConstantRange pushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eVertex, .offset = 0, .size = sizeof(PushConstant)};

  pipelineLayoutInfo.push_constants.push_back(pushConstantRange);
  pipelineLayoutInfo.descriptor_sets.push_back(static_descriptor_set_layout_->get());
  pipelineLayoutInfo.descriptor_sets.push_back(frame_descriptor_set_layout_->get());

  pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipelineLayoutInfo);

  pipelineLayoutInfo.descriptor_sets.clear();

  debug_line_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipelineLayoutInfo);

  // -----------------------------------------------------------
  // CREATE GRAPHICS PIPELINE
  // -----------------------------------------------------------
  {
    GraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.shader_stages = {vertStage, fragStage};
    pipelineInfo.color_attachment_formats = {vk::Format::eR32Uint};
    pipelineInfo.layout = pipeline_layout_->get();
    pipelineInfo.color_blend_attachments = {{}};
    pipelineInfo.front_face = vk::FrontFace::eClockwise;

    pipelineInfo.depth_attachment_format = vk::Format::eD32Sfloat;

    vk::VertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription positionAttr{};
    positionAttr.location = 0;
    positionAttr.binding = 0;
    positionAttr.format = vk::Format::eR32G32B32Sfloat;
    positionAttr.offset = offsetof(Vertex, position);

    // vk::VertexInputAttributeDescription colorAttr{};
    // colorAttr.location = 1;
    // colorAttr.binding = 0;
    // colorAttr.format = vk::Format::eR32G32B32Sfloat;
    // colorAttr.offset = offsetof(Vertex, color);
    //
    // vk::VertexInputAttributeDescription normalAttr{};
    // normalAttr.location = 2;
    // normalAttr.binding = 0;
    // normalAttr.format = vk::Format::eR32G32B32Sfloat;
    // normalAttr.offset = offsetof(Vertex, normal);

    pipelineInfo.vertex_bindings.push_back(binding);
    pipelineInfo.vertex_attributes.push_back(positionAttr);
    // pipelineInfo.vertexAttributes.push_back(colorAttr);
    // pipelineInfo.vertexAttributes.push_back(normalAttr);

    pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), pipelineInfo);
  }
  // -----------------------------------------------------------
  // CREATE DEBUG LINE GRAPHICS PIPELINE
  // -----------------------------------------------------------
  {
    GraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.shader_stages = {debugLineVertStage, debugLineFragStage};
    pipelineInfo.color_attachment_formats = {vk::Format::eB8G8R8A8Unorm};
    pipelineInfo.layout = pipeline_layout_->get();
    pipelineInfo.color_blend_attachments = {{}};
    pipelineInfo.topology = vk::PrimitiveTopology::eLineList;

    vk::VertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(DebugLineVertex);
    binding.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription positionAttr{};
    positionAttr.location = 0;
    positionAttr.binding = 0;
    positionAttr.format = vk::Format::eR32G32B32Sfloat;
    positionAttr.offset = offsetof(Vertex, position);

    vk::VertexInputAttributeDescription colorAttr{};
    colorAttr.location = 1;
    colorAttr.binding = 0;
    colorAttr.format = vk::Format::eR32G32B32Sfloat;
    colorAttr.offset = offsetof(Vertex, color);

    pipelineInfo.vertex_bindings.push_back(binding);
    pipelineInfo.vertex_attributes.push_back(positionAttr);
    pipelineInfo.vertex_attributes.push_back(colorAttr);

    debug_line_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), pipelineInfo);
  }

  // -----------------------------------------------------------
  // LOAD AND CREATE COMPUTE SHADER
  // -----------------------------------------------------------
  const auto compCode =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.comp.spv", ShaderResourceLoader{});
  compute_shader_ = std::make_unique<VulkanShader>(device_->get(), compCode->code);

  const vk::PipelineShaderStageCreateInfo compStage{
      .stage = vk::ShaderStageFlagBits::eCompute, .module = compute_shader_->get(), .pName = "main"};

  // -----------------------------------------------------------
  // CREATE COMPUTE PIPELINE LAYOUT (DESCRIPTOR SET LAYOUT)
  // -----------------------------------------------------------
  pipelineLayoutInfo.push_constants.clear();
  pipelineLayoutInfo.descriptor_sets.clear();

  pipelineLayoutInfo.descriptor_sets.push_back(static_descriptor_set_layout_->get());
  pipelineLayoutInfo.descriptor_sets.push_back(frame_descriptor_set_layout_->get());

  constexpr vk::PushConstantRange computePushConstantRange{
      .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(ComputePushConstant)};

  pipelineLayoutInfo.push_constants.push_back(computePushConstantRange);

  comp_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipelineLayoutInfo);
  // -----------------------------------------------------------
  // CREATE COMPUTE PIPELINE
  // -----------------------------------------------------------
  ComputePipelineInfo compPipelineInfo{
      .shader_stage = compStage,
      .layout = comp_pipeline_layout_->get(),
  };

  comp_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), compPipelineInfo);

  // -----------------------------------------------------------
  // CREATE DESCRIPTOR POOL
  // -----------------------------------------------------------
  DescriptorPoolInfo descriptorPoolInfo{};
  descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  descriptorPoolInfo.max_sets = MAX_DESCRIPTOR_SETS;
  descriptorPoolInfo.pool_sizes = {
      {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1},
      {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = STORAGE_BUFFER_COUNT}};

  descriptor_pool_ = std::make_unique<VulkanDescriptorPool>(device_->get(), descriptorPoolInfo);

  // -----------------------------------------------------------
  // CREATE FRAME RESOURCES
  // -----------------------------------------------------------
  frames_.reserve(max_frames_in_flight_);
  for (uint32_t i{}; i < max_frames_in_flight_; i++)
  {
    frames_.push_back(std::make_unique<VulkanFrame>(graphics_pool_.get(), compute_pool_.get(), descriptor_pool_.get(),
                                                     frame_descriptor_set_layout_.get(), device_.get(),
                                                     allocator_.get()));
  }

  const auto swapchainImageCount = swap_chain_->imageCount();
  for (uint32_t i{}; i < swapchainImageCount; i++)
  {
    constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    submit_semaphores_.push_back(device_->get().createSemaphoreUnique(semaphoreCreateInfo));
  }
  // -----------------------------------------------------------
  // CREATE SHARED RESOURCES
  // -----------------------------------------------------------

  RecreateDepthImage(width, height);

  // -----------------------------------------------------------
  // ALLOCATE STATIC DESCRIPTOR SET
  // -----------------------------------------------------------
  static_descriptor_set_ = descriptor_pool_->allocate(static_descriptor_set_layout_->get());

  // -----------------------------------------------------------
  // WRITE TO DESCRIPTOR SETS
  // -----------------------------------------------------------
  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(frames_.size() * 3);
  std::vector<vk::DescriptorBufferInfo> bufferInfos;
  bufferInfos.reserve(frames_.size() * 3);

  for (const auto& frame: frames_)
  {
    bufferInfos.push_back({.buffer = frame->IndirectBuffer()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write{.dstSet = frame->DescriptorSet(),
                                       .dstBinding = 0,
                                       .dstArrayElement = 0,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eStorageBuffer,
                                       .pBufferInfo = &bufferInfos.back()};

    writes.push_back(write);

    bufferInfos.push_back({.buffer = frame->ObjectBuffer()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write2{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 1,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                                        .pBufferInfo = &bufferInfos.back()};

    writes.push_back(write2);

    bufferInfos.push_back({.buffer = frame->DrawCount()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write3{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 2,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                                        .pBufferInfo = &bufferInfos.back()};

    writes.push_back(write3);
  }

  device_->get().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

  // -----------------------------------------------------------
  // INITIALIZE ImGui
  // -----------------------------------------------------------
  auto format = vk::Format::eB8G8R8A8Unorm;
  vk::PipelineRenderingCreateInfo renderingInfo{.colorAttachmentCount = 1, .pColorAttachmentFormats = &format};

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = instance_->get();
  initInfo.PhysicalDevice = device_->GetPhysical();
  initInfo.Device = device_->get();
  initInfo.QueueFamily = graphicsQueueFamily;
  initInfo.Queue = device_->GraphicsQueue();
  initInfo.DescriptorPool = descriptor_pool_->get();
  initInfo.MinImageCount = max_frames_in_flight_;
  initInfo.ImageCount = frames_.size();
  initInfo.UseDynamicRendering = true;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingInfo;

  ImGui_ImplVulkan_Init(&initInfo);
  util::println("Initialized renderer");
}

VulkanRenderer::~VulkanRenderer()
{
  device_->WaitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void VulkanRenderer::run(glm::mat4 world, float fov)
{
  ZoneScopedN("RenderLoop");

  // -----------------------------------------------------------
  // Handle window resize event
  // -----------------------------------------------------------
  for (const auto& event: event_manager_->getEvents())
  {
    switch (event.type)
    {
      case EventType::kWindowResized:
      {
        RecreateSwapChain();
        break;
      }
      default:
        break;
    }
  }

  // -----------------------------------------------------------
  // Begin frame
  // -----------------------------------------------------------

  const auto imageIndex = BeginFrame().value_or(UINT32_MAX);

  if (imageIndex == UINT32_MAX)
  {
    throw std::runtime_error("imageIndex is UINT32_MAX");
  }

  const auto& frame = frames_.at(current_frame_);

  // -----------------------------------------------------------
  // Upload render objects
  // -----------------------------------------------------------
  ZoneNamedN(objectszone, "UploadObjects", true);
  frame->ObjectBuffer()->WriteRange(render_objects_.data(), sizeof(RenderObject) * render_objects_.size());

  frame->DebugLineVertexBuffer()->WriteRange(debug_line_vertices_.data(),
                                             sizeof(DebugLineVertex) * debug_line_vertices_.size());

  // -----------------------------------------------------------
  // Calculate view, projection and frustum
  // -----------------------------------------------------------
  ZoneNamedN(matrixzone, "Matrices", true);
  const auto view = glm::inverse(world);
  const auto projection = glm::perspective(glm::radians(fov), aspect_ratio_, NEAR_PLANE_DISTANCE, FAR_PLANE_DISTANCE);

  const auto viewProj = projection * view;
  const auto frustum = ExtractFrustum(viewProj);

  // -----------------------------------------------------------
  // Compute pass - create indirect draw commands
  // -----------------------------------------------------------
  ZoneNamedN(computezone, "ComputePass", true);
  const auto ccmd = frame->ComputeCmd();
  constexpr vk::CommandBufferBeginInfo beginInfo{};
  ccmd.begin(beginInfo);

  constexpr vk::DebugUtilsLabelEXT labelInfo1{.pLabelName = "FrustumGPUDrivenPass"};
  ccmd.beginDebugUtilsLabelEXT(labelInfo1, instance_->getDynamicLoader());

  ccmd.fillBuffer(frame->DrawCount()->get(), 0, sizeof(uint32_t), 0);

  vulkan_barriers::BufferBarrier(
      ccmd, vulkan_barriers::BufferInfo{.buffer = frame->DrawCount()->get(), .size = sizeof(uint32_t)},
      vulkan_barriers::BufferUsageBit::CopyDestination, vulkan_barriers::BufferUsageBit::RWCompute);

  const auto descriptorSets = std::array{static_descriptor_set_, frame->DescriptorSet()};

  ccmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, comp_pipeline_layout_->get(), 0, descriptorSets.size(),
                          descriptorSets.data(), 0, nullptr);

  const ComputePushConstant computePushConstant{
      .frustum = frustum,
  };

  ccmd.pushConstants(comp_pipeline_layout_->get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputePushConstant),
                     &computePushConstant);

  ccmd.bindPipeline(vk::PipelineBindPoint::eCompute, comp_pipeline_->get());
  const uint32_t workgroups = (render_objects_.size() + 255) / 256;
  ccmd.dispatch(workgroups, 1, 1);

  ccmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());
  ccmd.end();

  // -----------------------------------------------------------
  // Graphics pass - render meshes
  // -----------------------------------------------------------
  ZoneNamedN(graphicsmesheszone, "MeshPass", true);
  const auto cmd = frame->GraphicsCmd();

  cmd.begin(beginInfo);

  constexpr vk::DebugUtilsLabelEXT labelInfo2{.pLabelName = "MeshPass"};
  cmd.beginDebugUtilsLabelEXT(labelInfo2, instance_->getDynamicLoader());

  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  const vk::RenderingAttachmentInfo depthAttachment{
      .imageView = frame->DepthImage()->view(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = vk::ClearValue{.depthStencil = {.depth = 1.0F, .stencil = 0}}};

  const vk::RenderingAttachmentInfo colorAttachment{
      .imageView = frame->VisibilityImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearValue{.color = vk::ClearColorValue{.uint32 = std::array{0U, 0U, 0U, 0U}}}};

  const vk::RenderingInfo renderInfo{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment};

  cmd.beginRendering(renderInfo);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_->get());

  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout_->get(), 0, descriptorSets.size(),
                         descriptorSets.data(), 0, nullptr);

  const PushConstant pushConstant{
      .view = view,
      .proj = projection,
  };

  cmd.pushConstants(pipeline_layout_->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant), &pushConstant);
  const vk::DeviceSize offset = 0;
  const auto vertexBuffer = vertex_buffer_->get();
  cmd.bindVertexBuffers(0, 1, &vertexBuffer, &offset);
  const auto indexBuffer = index_buffer_->get();
  cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

  const vk::Viewport viewport{.x = 0.0F,
                              .y = 0.0F,
                              .width = static_cast<float>(swap_chain_->extent().width),
                              .height = static_cast<float>(swap_chain_->extent().height),
                              .minDepth = 0.0F,
                              .maxDepth = 1.0F};
  cmd.setViewport(0, 1, &viewport);

  const vk::Rect2D scissor{.offset = {.x = 0, .y = 0},
                           .extent = {.width = swap_chain_->extent().width, .height = swap_chain_->extent().height}};
  cmd.setScissor(0, 1, &scissor);

  cmd.drawIndexedIndirectCount(frame->IndirectBuffer()->get(), 0, frame->DrawCount()->get(), 0, render_objects_.size(),
                               sizeof(vk::DrawIndexedIndirectCommand));
  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  // -----------------------------------------------------------
  // Graphics pass - render debug lines
  // -----------------------------------------------------------
  ZoneNamedN(graphicslineszone, "DebugLinesPass", true);

  constexpr vk::DebugUtilsLabelEXT labelInfo3{.pLabelName = "DebugLinesPass"};
  cmd.beginDebugUtilsLabelEXT(labelInfo3, instance_->getDynamicLoader());

  const vk::RenderingAttachmentInfo debugLineColorAttachment{
      .imageView = frame->RenderImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
  };

  const vk::RenderingInfo debugLineRenderInfo{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &debugLineColorAttachment};

  cmd.beginRendering(debugLineRenderInfo);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, debug_line_pipeline_->get());

  cmd.pushConstants(debug_line_pipeline_layout_->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant),
                    &pushConstant);
  const auto debugLineVertexBuffer = frame->DebugLineVertexBuffer()->get();
  cmd.bindVertexBuffers(0, 1, &debugLineVertexBuffer, &offset);

  cmd.setViewport(0, 1, &viewport);

  cmd.setScissor(0, 1, &scissor);

  cmd.draw(debug_line_vertices_.size(), 1, 0, 0);

  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  // -----------------------------------------------------------
  // ImGui pass
  // -----------------------------------------------------------
  ZoneNamedN(imguizone, "ImGui", true);

  constexpr vk::DebugUtilsLabelEXT labelInfo4{.pLabelName = "ImGuiPass"};
  cmd.beginDebugUtilsLabelEXT(labelInfo4, instance_->getDynamicLoader());

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("uhh");
  ImGui::End();

  ImGui::Render();

  const vk::RenderingAttachmentInfo imguiColorAttachment{
      .imageView = frame->RenderImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad, // Load existing scene
      .storeOp = vk::AttachmentStoreOp::eStore,
  };

  const vk::RenderingInfo imguiRenderInfo{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &imguiColorAttachment};

  cmd.beginRendering(imguiRenderInfo);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  ZoneNamedN(blitzone, "Blitting", true);
  constexpr vk::DebugUtilsLabelEXT labelInfo5{.pLabelName = "BlittingPass"};
  cmd.beginDebugUtilsLabelEXT(labelInfo5, instance_->getDynamicLoader());

  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::eTransferSrcOptimal);
  VulkanImage::TransitionImageLayout(swap_chain_->getImage(imageIndex), cmd, vk::ImageLayout::ePresentSrcKHR,
                                     vk::ImageLayout::eTransferDstOptimal);

  const vk::ImageBlit blitRegion{.srcSubresource =
                                     {
                                         .aspectMask = vk::ImageAspectFlagBits::eColor,
                                         .mipLevel = 0,
                                         .baseArrayLayer = 0,
                                         .layerCount = 1,
                                     },
                                 .srcOffsets = {{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                                 vk::Offset3D{.x = static_cast<int32_t>(swap_chain_->extent().width),
                                                              .y = static_cast<int32_t>(swap_chain_->extent().height),
                                                              .z = 1}}},
                                 .dstSubresource =
                                     {
                                         .aspectMask = vk::ImageAspectFlagBits::eColor,
                                         .mipLevel = 0,
                                         .baseArrayLayer = 0,
                                         .layerCount = 1,
                                     },
                                 .dstOffsets = {{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                                 vk::Offset3D{.x = static_cast<int32_t>(swap_chain_->extent().width),
                                                              .y = static_cast<int32_t>(swap_chain_->extent().height),
                                                              .z = 1}}}};

  cmd.blitImage(frame->RenderImage()->get(), vk::ImageLayout::eTransferSrcOptimal, swap_chain_->getImage(imageIndex),
                vk::ImageLayout::eTransferDstOptimal, 1U, &blitRegion, vk::Filter::eNearest);

  VulkanImage::TransitionImageLayout(swap_chain_->getImage(imageIndex), cmd, vk::ImageLayout::eTransferDstOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  cmd.end();

  // -----------------------------------------------------------
  // End frame
  // -----------------------------------------------------------
  ZoneNamedN(endzone, "Endzone", true);
  EndFrame(imageIndex);
}

uint32_t VulkanRenderer::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                                 uint32_t first_index, int32_t vertex_offset, const glm::vec3& b_min,
                                 const glm::vec3& b_max)
{
  const uint32_t meshID = mesh_infos_.size();
  vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
  indices_.insert(indices_.end(), indices.begin(), indices.end());
  mesh_infos_.emplace_back(b_min, indices.size(), b_max, first_index, vertex_offset);
  return meshID;
}

void VulkanRenderer::RenderLine(const glm::vec3& point_a, const glm::vec3& point_b, const glm::vec3& color)
{
  debug_line_vertices_.emplace_back(point_a, color);
  debug_line_vertices_.emplace_back(point_b, color);
}

void VulkanRenderer::ClearLines() { debug_line_vertices_.clear(); }

void VulkanRenderer::Upload()
{
  device_->WaitIdle();

  if (vertex_buffer_)
  {
    vertex_buffer_->Destroy();
    vertex_buffer_ = nullptr;
  }

  if (index_buffer_)
  {
    index_buffer_->Destroy();
    index_buffer_ = nullptr;
  }

  if (mesh_info_buffer_)
  {
    mesh_info_buffer_->Destroy();
    mesh_info_buffer_ = nullptr;
  }

  const auto verticesSize = sizeof(Vertex) * vertices_.size();
  const auto indicesSize = sizeof(uint32_t) * indices_.size();
  const auto meshesSize = sizeof(MeshInfo) * mesh_infos_.size();

  vertex_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = verticesSize,
                 .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  index_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = indicesSize,
                 .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  mesh_info_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = meshesSize,
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  auto stagingVertexBuffer =
      VulkanBuffer(BufferInfo{.size = verticesSize,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  stagingVertexBuffer.Write(vertices_.data());

  auto stagingIndexBuffer =
      VulkanBuffer(BufferInfo{.size = indicesSize,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  stagingIndexBuffer.Write(indices_.data());

  auto stagingMeshInfoBuffer =
      VulkanBuffer(BufferInfo{.size = meshesSize,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  stagingMeshInfoBuffer.Write(mesh_infos_.data());

  auto cmd = util::BeginSingleTimeCommandBuffer(*transfer_pool_);

  vk::BufferCopy vertexCopyRegion = {};
  vertexCopyRegion.srcOffset = 0;
  vertexCopyRegion.dstOffset = 0;
  vertexCopyRegion.size = verticesSize;
  cmd.copyBuffer(stagingVertexBuffer.get(), vertex_buffer_->get(), 1, &vertexCopyRegion);

  vk::BufferCopy indexCopyRegion = {};
  indexCopyRegion.srcOffset = 0;
  indexCopyRegion.dstOffset = 0;
  indexCopyRegion.size = indicesSize;
  cmd.copyBuffer(stagingIndexBuffer.get(), index_buffer_->get(), 1, &indexCopyRegion);

  vk::BufferCopy meshInfoCopyRegion = {};
  meshInfoCopyRegion.srcOffset = 0;
  meshInfoCopyRegion.dstOffset = 0;
  meshInfoCopyRegion.size = meshesSize;
  cmd.copyBuffer(stagingMeshInfoBuffer.get(), mesh_info_buffer_->get(), 1, &meshInfoCopyRegion);

  if (device_->QueueFamilies().transfer != device_->QueueFamilies().graphics)
  {
    std::array<vk::BufferMemoryBarrier2, 2> releaseBarriers = {
        {{.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
          .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
          .dstStageMask = vk::PipelineStageFlagBits2::eNone,
          .dstAccessMask = vk::AccessFlagBits2::eNone,
          .srcQueueFamilyIndex = device_->QueueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = device_->QueueFamilies().graphics.value_or(0),
          .buffer = vertex_buffer_->get(),
          .offset = 0,
          .size = vk::WholeSize},
         {.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
          .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
          .dstStageMask = vk::PipelineStageFlagBits2::eNone,
          .dstAccessMask = vk::AccessFlagBits2::eNone,
          .srcQueueFamilyIndex = device_->QueueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = device_->QueueFamilies().graphics.value_or(0),
          .buffer = index_buffer_->get(),
          .offset = 0,
          .size = vk::WholeSize}}};

    const vk::DependencyInfoKHR dependencyInfo{.sType = vk::StructureType::eDependencyInfoKHR,
                                               .bufferMemoryBarrierCount = 2,
                                               .pBufferMemoryBarriers = releaseBarriers.data()};

    cmd.pipelineBarrier2(dependencyInfo);
  }
  util::EndSingleTimeCommandBuffer(cmd, device_->TransferQueue(), *transfer_pool_);

  // UPDATE MESH INFO DESCRIPTOR SET
  auto bufferInfo = vk::DescriptorBufferInfo{.buffer = mesh_info_buffer_->get(), .offset = 0, .range = vk::WholeSize};

  const vk::WriteDescriptorSet write{.dstSet = static_descriptor_set_,
                                     .dstBinding = 0,
                                     .dstArrayElement = 0,
                                     .descriptorCount = 1,
                                     .descriptorType = vk::DescriptorType::eStorageBuffer,
                                     .pBufferInfo = &bufferInfo};
  device_->get().updateDescriptorSets(1, &write, 0, nullptr);

  if (device_->QueueFamilies().transfer != device_->QueueFamilies().graphics)
  {
    auto graphicsCmd = util::BeginSingleTimeCommandBuffer(*graphics_pool_);

    std::array<vk::BufferMemoryBarrier2, 2> acquireBarriers = {
        {{.srcStageMask = vk::PipelineStageFlagBits2::eNone,
          .srcAccessMask = vk::AccessFlagBits2::eNone,
          .dstStageMask = vk::PipelineStageFlagBits2::eVertexInput,
          .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
          .srcQueueFamilyIndex = device_->QueueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = device_->QueueFamilies().graphics.value_or(0),
          .buffer = vertex_buffer_->get(),
          .offset = 0,
          .size = vk::WholeSize},
         {.srcStageMask = vk::PipelineStageFlagBits2::eNone,
          .srcAccessMask = vk::AccessFlagBits2::eNone,
          .dstStageMask = vk::PipelineStageFlagBits2::eIndexInput,
          .dstAccessMask = vk::AccessFlagBits2::eIndexRead,
          .srcQueueFamilyIndex = device_->QueueFamilies().transfer.value_or(0),
          .dstQueueFamilyIndex = device_->QueueFamilies().graphics.value_or(0),
          .buffer = index_buffer_->get(),
          .offset = 0,
          .size = vk::WholeSize}}};

    const vk::DependencyInfo acquireDependencyInfo{.bufferMemoryBarrierCount = 2,
                                                   .pBufferMemoryBarriers = acquireBarriers.data()};

    graphicsCmd.pipelineBarrier2(acquireDependencyInfo);

    util::EndSingleTimeCommandBuffer(graphicsCmd, device_->GraphicsQueue(), *graphics_pool_);
  }
}

void VulkanRenderer::RenderMesh(glm::mat4 model, const uint32_t mesh_id) { render_objects_.emplace_back(model, mesh_id); }

void VulkanRenderer::ClearMeshes() { render_objects_.clear(); }

int32_t VulkanRenderer::GetVertexCount() const { return static_cast<int32_t>(vertices_.size()); }
uint32_t VulkanRenderer::GetIndexCount() const { return indices_.size(); }

std::optional<uint32_t> VulkanRenderer::BeginFrame() const
{
  ZoneScopedN("VulkanRenderer::beginFrame");
  const auto& frame = frames_.at(current_frame_);

  auto result = device_->get().waitForFences(frame->InFlight(), VK_TRUE, UINT64_MAX);
  if (result != vk::Result::eSuccess)
  {
    throw std::runtime_error("waitForFences failed: " + vk::to_string(result));
  }

  uint32_t imageIndex{};
  result = swap_chain_->acquireNextImage(frame->ImageAvailable(), imageIndex);

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    return std::nullopt;
  }

  device_->get().resetFences(frame->InFlight());

  return imageIndex;
}

auto VulkanRenderer::EndFrame(const uint32_t image_index) -> void
{
  ZoneScopedN("VulkanRenderer::endFrame");
  auto& frame = frames_.at(current_frame_);

  const vk::SemaphoreSubmitInfo cSignalSemaphore{.semaphore = frame->ComputeFinished(),
                                                 .stageMask = vk::PipelineStageFlagBits2::eComputeShader};

  const vk::CommandBufferSubmitInfo cCmdInfo{.commandBuffer = frame->ComputeCmd()};

  const vk::SubmitInfo2 cSubmitInfo{.waitSemaphoreInfoCount = 0,
                                    .pWaitSemaphoreInfos = nullptr,
                                    .commandBufferInfoCount = 1,
                                    .pCommandBufferInfos = &cCmdInfo,
                                    .signalSemaphoreInfoCount = 1,
                                    .pSignalSemaphoreInfos = &cSignalSemaphore};

  auto result = device_->ComputeQueue().submit2(1, &cSubmitInfo, nullptr);
  if (result != vk::Result::eSuccess)
  {
    throw std::runtime_error("compute submit2 failed: " + vk::to_string(result));
  }

  std::array<vk::SemaphoreSubmitInfo, 2> gWaitSemaphores{
      {{.semaphore = frame->ComputeFinished(), .stageMask = vk::PipelineStageFlagBits2::eDrawIndirect},
       {.semaphore = frame->ImageAvailable(), .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput}}};

  const vk::SemaphoreSubmitInfo gSignalSemaphore{.semaphore = submit_semaphores_.at(image_index).get(),
                                                 .stageMask = vk::PipelineStageFlagBits2::eAllCommands};

  const vk::CommandBufferSubmitInfo gCmdInfo{.commandBuffer = frame->GraphicsCmd()};

  const vk::SubmitInfo2 gSubmitInfo{.waitSemaphoreInfoCount = gWaitSemaphores.size(),
                                    .pWaitSemaphoreInfos = gWaitSemaphores.data(),
                                    .commandBufferInfoCount = 1,
                                    .pCommandBufferInfos = &gCmdInfo,
                                    .signalSemaphoreInfoCount = 1,
                                    .pSignalSemaphoreInfos = &gSignalSemaphore};

  result = device_->GraphicsQueue().submit2(1, &gSubmitInfo, frame->InFlight());
  if (result != vk::Result::eSuccess)
  {
    throw std::runtime_error("graphics submit2 failed: " + vk::to_string(result));
  }

  result = swap_chain_->present(image_index, device_->PresentQueue(), submit_semaphores_.at(image_index).get());

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    RecreateSwapChain();
  }

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}
void VulkanRenderer::RecreateSwapChain()
{
  const auto [width, height] = window_->GetWindowSize();

  swap_chain_->recreate({.width = width, .height = height});

  RecreateDepthImage(width, height);

  aspect_ratio_ = static_cast<float>(width) / static_cast<float>(height);
}
void VulkanRenderer::RecreateDepthImage(const uint32_t width, const uint32_t height) const
{
  for (const auto& frame: frames_)
  {
    frame->RecreateFrameImages(width, height);
  }
}

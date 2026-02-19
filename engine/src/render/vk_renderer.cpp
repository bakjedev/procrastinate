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

constexpr float kNearPlaneDistance = 0.01F;
constexpr float kFarPlaneDistance = 1000.0F;
constexpr uint32_t kMaxDescriptorSets = 1000;
constexpr uint32_t kStorageBufferCount = 20;
constexpr uint32_t kStorageImageCount = 20;
constexpr uint32_t kCombinedImageSamplerCount = 20;

VulkanRenderer::VulkanRenderer(Window* window, ResourceManager& resource_manager, EventManager& event_manager) :
    window_(window), event_manager_(&event_manager)
{
  util::println("Initializing renderer");

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
  // CREATE COMMAND POOLS FOR GRAPHICS AND TRANSFER
  // -----------------------------------------------------------
  const uint32_t graphics_queue_family = device_->QueueFamilies().graphics.value();
  const uint32_t transfer_queue_family = device_->QueueFamilies().transfer.value();

  graphics_pool_ =
      std::make_unique<VulkanCommandPool>(CommandPoolInfo{.queue_family_index = graphics_queue_family,
                                                          .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
                                          device_->get());

  transfer_pool_ = std::make_unique<VulkanCommandPool>(
      CommandPoolInfo{.queue_family_index = transfer_queue_family, .flags = {}}, device_->get());

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
  // CREATE VISIBILITY SAMPLER
  // -----------------------------------------------------------
  vk::SamplerCreateInfo sampler_create_info{
      .magFilter = vk::Filter::eNearest,
      .minFilter = vk::Filter::eNearest,
      .mipmapMode = vk::SamplerMipmapMode::eNearest,
      .addressModeU = vk::SamplerAddressMode::eRepeat,
      .addressModeV = vk::SamplerAddressMode::eRepeat,
      .addressModeW = vk::SamplerAddressMode::eRepeat,
      .mipLodBias = 0.0f,
      .anisotropyEnable = vk::False,
      .maxAnisotropy = 1.0f,
      .compareEnable = vk::False,
      .minLod = 0.0f,
      .maxLod = vk::LodClampNone,
      .borderColor = vk::BorderColor::eFloatOpaqueBlack,
      .unnormalizedCoordinates = vk::False,
  };

  visibility_sampler_ = device_->get().createSamplerUnique(sampler_create_info);

  // -----------------------------------------------------------
  // CREATE DESCRIPTOR POOL
  // -----------------------------------------------------------
  DescriptorPoolInfo descriptor_pool_info{};
  descriptor_pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  descriptor_pool_info.max_sets = kMaxDescriptorSets;
  descriptor_pool_info.pool_sizes = {
      {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = kCombinedImageSamplerCount},
      {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = kStorageBufferCount},
      {.type = vk::DescriptorType::eStorageImage, .descriptorCount = kStorageImageCount}};

  descriptor_pool_ = std::make_unique<VulkanDescriptorPool>(device_->get(), descriptor_pool_info);

  // -----------------------------------------------------------
  // CREATE DESCRIPTOR SET LAYOUTS
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
                                                 .stageFlags = vk::ShaderStageFlagBits::eCompute},
                  vk::DescriptorSetLayoutBinding{// visibilityImage
                                                 .binding = 3,
                                                 .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                                 .descriptorCount = 1,
                                                 .stageFlags = vk::ShaderStageFlagBits::eCompute},
                  vk::DescriptorSetLayoutBinding{// renderImage
                                                 .binding = 4,
                                                 .descriptorType = vk::DescriptorType::eStorageImage,
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

  static_descriptor_set_ = descriptor_pool_->allocate(static_descriptor_set_layout_->get());

  // -----------------------------------------------------------
  // CREATE FRAME RESOURCES
  // -----------------------------------------------------------
  frames_.reserve(max_frames_in_flight_);
  for (uint32_t i{}; i < max_frames_in_flight_; i++)
  {
    frames_.push_back(std::make_unique<VulkanFrame>(graphics_pool_.get(), descriptor_pool_.get(),
                                                    frame_descriptor_set_layout_.get(), device_.get(),
                                                    allocator_.get()));
  }

  // submit semaphores
  const auto swap_chain_image_count = swap_chain_->imageCount();
  for (uint32_t i{}; i < swap_chain_image_count; i++)
  {
    constexpr vk::SemaphoreCreateInfo semaphore_create_info{};
    submit_semaphores_.push_back(device_->get().createSemaphoreUnique(semaphore_create_info));
  }

  RecreateFrameImages(width, height);

  // Transition render images
  {
    auto cmd = util::BeginSingleTimeCommandBuffer(*graphics_pool_);
    for (const auto& frame: frames_)
    {
      VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::eTransferSrcOptimal);
      VulkanImage::TransitionImageLayout(frame->VisibilityImage()->get(), cmd, vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
    }
    util::EndSingleTimeCommandBuffer(cmd, device_->GraphicsQueue(), *graphics_pool_);
  }

  // -----------------------------------------------------------
  // WRITE TO DESCRIPTOR SETS
  // -----------------------------------------------------------
  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(frames_.size() * 5);
  std::vector<vk::DescriptorBufferInfo> buffer_infos;
  buffer_infos.reserve(frames_.size() * 3);
  std::vector<vk::DescriptorImageInfo> image_infos;
  image_infos.reserve(frames_.size() * 2);

  for (const auto& frame: frames_)
  {
    buffer_infos.push_back({.buffer = frame->IndirectBuffer()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write{.dstSet = frame->DescriptorSet(),
                                       .dstBinding = 0,
                                       .dstArrayElement = 0,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eStorageBuffer,
                                       .pBufferInfo = &buffer_infos.back()};

    writes.push_back(write);

    buffer_infos.push_back({.buffer = frame->ObjectBuffer()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write2{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 1,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                                        .pBufferInfo = &buffer_infos.back()};

    writes.push_back(write2);

    buffer_infos.push_back({.buffer = frame->DrawCount()->get(), .offset = 0, .range = vk::WholeSize});

    const vk::WriteDescriptorSet write3{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 2,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                                        .pBufferInfo = &buffer_infos.back()};

    writes.push_back(write3);

    image_infos.push_back(vk::DescriptorImageInfo{.sampler = visibility_sampler_.get(),
                                                  .imageView = frame->VisibilityImage()->view(),
                                                  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal});

    const vk::WriteDescriptorSet write4{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 3,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                        .pImageInfo = &image_infos.back()};

    writes.push_back(write4);

    image_infos.push_back(vk::DescriptorImageInfo{.sampler = VK_NULL_HANDLE,
                                                  .imageView = frame->RenderImage()->view(),
                                                  .imageLayout = vk::ImageLayout::eGeneral});

    const vk::WriteDescriptorSet write5{.dstSet = frame->DescriptorSet(),
                                        .dstBinding = 4,
                                        .dstArrayElement = 0,
                                        .descriptorCount = 1,
                                        .descriptorType = vk::DescriptorType::eStorageImage,
                                        .pImageInfo = &image_infos.back()};

    writes.push_back(write5);
  }

  device_->get().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

  // -----------------------------------------------------------
  // LOAD SHADERS
  // -----------------------------------------------------------

  // pre pass
  const auto vert_code =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.vert.spv", ShaderResourceLoader{});
  pre_pass_vert_ = std::make_unique<VulkanShader>(device_->get(), vert_code->code);

  const auto frag_code =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.frag.spv", ShaderResourceLoader{});
  pre_pass_frag_ = std::make_unique<VulkanShader>(device_->get(), frag_code->code);

  const vk::PipelineShaderStageCreateInfo vert_stage{
      .stage = vk::ShaderStageFlagBits::eVertex, .module = pre_pass_vert_->get(), .pName = "main"};

  const vk::PipelineShaderStageCreateInfo frag_stage{
      .stage = vk::ShaderStageFlagBits::eFragment, .module = pre_pass_frag_->get(), .pName = "main"};

  // debug line
  const auto debug_line_vert_code = resource_manager.createFromFile<ShaderResource>(
      "engine/assets/shaders/debug_line.vert.spv", ShaderResourceLoader{});
  debug_line_vert_ = std::make_unique<VulkanShader>(device_->get(), debug_line_vert_code->code);

  const auto debug_line_frag_code = resource_manager.createFromFile<ShaderResource>(
      "engine/assets/shaders/debug_line.frag.spv", ShaderResourceLoader{});
  debug_line_frag_ = std::make_unique<VulkanShader>(device_->get(), debug_line_frag_code->code);

  const vk::PipelineShaderStageCreateInfo debug_line_vert_stage{
      .stage = vk::ShaderStageFlagBits::eVertex, .module = debug_line_vert_->get(), .pName = "main"};

  const vk::PipelineShaderStageCreateInfo debug_line_frag_stage{
      .stage = vk::ShaderStageFlagBits::eFragment, .module = debug_line_frag_->get(), .pName = "main"};

  // culling
  const auto culling_comp_code =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/test.comp.spv", ShaderResourceLoader{});
  culling_comp_ = std::make_unique<VulkanShader>(device_->get(), culling_comp_code->code);

  const vk::PipelineShaderStageCreateInfo culling_comp_stage{
      .stage = vk::ShaderStageFlagBits::eCompute, .module = culling_comp_->get(), .pName = "main"};

  // shading
  const auto shading_comp_code =
      resource_manager.createFromFile<ShaderResource>("engine/assets/shaders/shading.comp.spv", ShaderResourceLoader{});
  shading_comp_ = std::make_unique<VulkanShader>(device_->get(), shading_comp_code->code);

  const vk::PipelineShaderStageCreateInfo shading_comp_stage{
      .stage = vk::ShaderStageFlagBits::eCompute, .module = shading_comp_->get(), .pName = "main"};

  // -----------------------------------------------------------
  // CREATE PIPELINE LAYOUTS
  // -----------------------------------------------------------
  constexpr vk::PushConstantRange push_constant_range{
      .stageFlags = vk::ShaderStageFlagBits::eVertex, .offset = 0, .size = sizeof(PushConstant)};

  // pre pass
  {
    PipelineLayoutInfo pipeline_layout_info{};

    pipeline_layout_info.push_constants.push_back(push_constant_range);
    pipeline_layout_info.descriptor_sets.push_back(static_descriptor_set_layout_->get());
    pipeline_layout_info.descriptor_sets.push_back(frame_descriptor_set_layout_->get());

    pre_pass_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipeline_layout_info);
  }

  // debug line
  {
    PipelineLayoutInfo pipeline_layout_info{};

    pipeline_layout_info.push_constants.push_back(push_constant_range);

    debug_line_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipeline_layout_info);
  }

  // culling
  {
    PipelineLayoutInfo pipeline_layout_info{};

    pipeline_layout_info.descriptor_sets.push_back(static_descriptor_set_layout_->get());
    pipeline_layout_info.descriptor_sets.push_back(frame_descriptor_set_layout_->get());

    constexpr vk::PushConstantRange compute_push_constant_range{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(ComputePushConstant)};

    pipeline_layout_info.push_constants.push_back(compute_push_constant_range);

    culling_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipeline_layout_info);
  }

  // shading
  {
    PipelineLayoutInfo pipeline_layout_info{};

    pipeline_layout_info.descriptor_sets.push_back(static_descriptor_set_layout_->get());
    pipeline_layout_info.descriptor_sets.push_back(frame_descriptor_set_layout_->get());

    shading_pipeline_layout_ = std::make_unique<VulkanPipelineLayout>(device_->get(), pipeline_layout_info);
  }

  // -----------------------------------------------------------
  // CREATE PIPELINES
  // -----------------------------------------------------------

  // pre pass
  {
    GraphicsPipelineInfo pipeline_info{};
    pipeline_info.shader_stages = {vert_stage, frag_stage};
    pipeline_info.color_attachment_formats = {vk::Format::eR32Uint};
    pipeline_info.layout = pre_pass_pipeline_layout_->get();
    pipeline_info.color_blend_attachments = {{}};
    pipeline_info.front_face = vk::FrontFace::eClockwise;

    pipeline_info.depth_attachment_format = vk::Format::eD32Sfloat;

    vk::VertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription position_attr{};
    position_attr.location = 0;
    position_attr.binding = 0;
    position_attr.format = vk::Format::eR32G32B32Sfloat;
    position_attr.offset = offsetof(Vertex, position);

    pipeline_info.vertex_bindings.push_back(binding);
    pipeline_info.vertex_attributes.push_back(position_attr);

    pre_pass_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), pipeline_info);
  }

  // debug line
  {
    GraphicsPipelineInfo pipeline_info{};
    pipeline_info.shader_stages = {debug_line_vert_stage, debug_line_frag_stage};
    pipeline_info.color_attachment_formats = {vk::Format::eB8G8R8A8Unorm};
    pipeline_info.layout = pre_pass_pipeline_layout_->get();
    pipeline_info.color_blend_attachments = {{}};
    pipeline_info.topology = vk::PrimitiveTopology::eLineList;

    vk::VertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(DebugLineVertex);
    binding.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription position_attr{};
    position_attr.location = 0;
    position_attr.binding = 0;
    position_attr.format = vk::Format::eR32G32B32Sfloat;
    position_attr.offset = offsetof(Vertex, position);

    vk::VertexInputAttributeDescription color_attr{};
    color_attr.location = 1;
    color_attr.binding = 0;
    color_attr.format = vk::Format::eR32G32B32Sfloat;
    color_attr.offset = offsetof(Vertex, color);

    pipeline_info.vertex_bindings.push_back(binding);
    pipeline_info.vertex_attributes.push_back(position_attr);
    pipeline_info.vertex_attributes.push_back(color_attr);

    debug_line_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), pipeline_info);
  }

  // culling
  {
    ComputePipelineInfo culling_pipeline_info{
        .shader_stage = culling_comp_stage,
        .layout = culling_pipeline_layout_->get(),
    };

    culling_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), culling_pipeline_info);
  }

  // shading
  {
    ComputePipelineInfo shading_pipeline_info{
        .shader_stage = shading_comp_stage,
        .layout = shading_pipeline_layout_->get(),
    };

    shading_pipeline_ = std::make_unique<VulkanPipeline>(device_->get(), shading_pipeline_info);
  }

  // -----------------------------------------------------------
  // INITIALIZE ImGui
  // -----------------------------------------------------------
  auto format = vk::Format::eB8G8R8A8Unorm;
  vk::PipelineRenderingCreateInfo rendering_info{.colorAttachmentCount = 1, .pColorAttachmentFormats = &format};

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance_->get();
  init_info.PhysicalDevice = device_->GetPhysical();
  init_info.Device = device_->get();
  init_info.QueueFamily = graphics_queue_family;
  init_info.Queue = device_->GraphicsQueue();
  init_info.DescriptorPool = descriptor_pool_->get();
  init_info.MinImageCount = max_frames_in_flight_;
  init_info.ImageCount = frames_.size();
  init_info.UseDynamicRendering = true;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo = rendering_info;

  ImGui_ImplVulkan_Init(&init_info);
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
  for (const auto& event: event_manager_->GetEvents())
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

  const auto image_index = BeginFrame().value_or(UINT32_MAX);

  if (image_index == UINT32_MAX)
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
  const auto projection = glm::perspective(glm::radians(fov), aspect_ratio_, kNearPlaneDistance, kFarPlaneDistance);

  const auto view_proj = projection * view;
  const auto frustum = ExtractFrustum(view_proj);

  // -----------------------------------------------------------
  // Begin command buffer
  // -----------------------------------------------------------
  const auto cmd = frame->GraphicsCmd();
  constexpr vk::CommandBufferBeginInfo begin_info{};
  cmd.begin(begin_info);

  // -----------------------------------------------------------
  // Compute pass - create indirect draw commands
  // -----------------------------------------------------------
  ZoneNamedN(computezone, "ComputePass", true);
  constexpr vk::DebugUtilsLabelEXT label_info1{.pLabelName = "FrustumGPUDrivenPass"};
  cmd.beginDebugUtilsLabelEXT(label_info1, instance_->getDynamicLoader());

  cmd.fillBuffer(frame->DrawCount()->get(), 0, sizeof(uint32_t), 0);

  vulkan_barriers::BufferBarrier(
      cmd, vulkan_barriers::BufferInfo{.buffer = frame->DrawCount()->get(), .size = sizeof(uint32_t)},
      vulkan_barriers::BufferUsageBit::CopyDestination, vulkan_barriers::BufferUsageBit::RWCompute);

  const auto descriptor_sets = std::array{static_descriptor_set_, frame->DescriptorSet()};

  cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, culling_pipeline_layout_->get(), 0, descriptor_sets.size(),
                         descriptor_sets.data(), 0, nullptr);

  const auto render_objects_size = static_cast<uint32_t>(render_objects_.size());
  const ComputePushConstant compute_push_constant{
      .frustum = frustum,
      .render_object_count = render_objects_size,
  };

  cmd.pushConstants(culling_pipeline_layout_->get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputePushConstant),
                    &compute_push_constant);

  cmd.bindPipeline(vk::PipelineBindPoint::eCompute, culling_pipeline_->get());
  const uint32_t workgroups = (render_objects_size + 255) / 256;
  cmd.dispatch(workgroups, 1, 1);

  vulkan_barriers::BufferBarrier(
      cmd, vulkan_barriers::BufferInfo{.buffer = frame->DrawCount()->get(), .size = sizeof(uint32_t)},
      vulkan_barriers::BufferUsageBit::RWCompute, vulkan_barriers::BufferUsageBit::IndirectDraw);

  vulkan_barriers::BufferBarrier(
      cmd, vulkan_barriers::BufferInfo{.buffer = frame->IndirectBuffer()->get(), .size = vk::WholeSize},
      vulkan_barriers::BufferUsageBit::RWCompute, vulkan_barriers::BufferUsageBit::IndirectDraw);

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  // -----------------------------------------------------------
  // Graphics pass - render meshes visibility
  // -----------------------------------------------------------
  ZoneNamedN(graphicsmesheszone, "MeshPass", true);
  constexpr vk::DebugUtilsLabelEXT label_info2{.pLabelName = "MeshPass"};
  cmd.beginDebugUtilsLabelEXT(label_info2, instance_->getDynamicLoader());

  VulkanImage::TransitionImageLayout(frame->VisibilityImage()->get(), cmd, vk::ImageLayout::eShaderReadOnlyOptimal,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eTransferSrcOptimal,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  const vk::RenderingAttachmentInfo depth_attachment{
      .imageView = frame->DepthImage()->view(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = vk::ClearValue{.depthStencil = {.depth = 1.0F, .stencil = 0}}};

  const vk::RenderingAttachmentInfo color_attachment{
      .imageView = frame->VisibilityImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearValue{.color = vk::ClearColorValue{.uint32 = std::array{0U, 0U, 0U, 0U}}}};

  const vk::RenderingInfo render_info{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment,
      .pDepthAttachment = &depth_attachment};

  cmd.beginRendering(render_info);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pre_pass_pipeline_->get());

  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pre_pass_pipeline_layout_->get(), 0, descriptor_sets.size(),
                         descriptor_sets.data(), 0, nullptr);

  const PushConstant push_constant{
      .view = view,
      .proj = projection,
  };

  cmd.pushConstants(pre_pass_pipeline_layout_->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant),
                    &push_constant);
  constexpr vk::DeviceSize offset = 0;
  const auto vertex_buffer = vertex_buffer_->get();
  cmd.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
  const auto index_buffer = index_buffer_->get();
  cmd.bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);

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

  cmd.drawIndexedIndirectCount(frame->IndirectBuffer()->get(), 0, frame->DrawCount()->get(), 0, render_objects_size,
                               sizeof(vk::DrawIndexedIndirectCommand));
  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  // -----------------------------------------------------------
  // Compute pass - shade render image with meshes
  // -----------------------------------------------------------
  ZoneNamedN(computeshadingzone, "ComputeShadingPass", true);
  constexpr vk::DebugUtilsLabelEXT label_info6{.pLabelName = "ShadingPass"};
  cmd.beginDebugUtilsLabelEXT(label_info6, instance_->getDynamicLoader());

  VulkanImage::TransitionImageLayout(frame->VisibilityImage()->get(), cmd, vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::eShaderReadOnlyOptimal);

  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::eGeneral);

  cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, shading_pipeline_layout_->get(), 0, descriptor_sets.size(),
                         descriptor_sets.data(), 0, nullptr);
  cmd.bindPipeline(vk::PipelineBindPoint::eCompute, shading_pipeline_->get());
  {
    const auto [width, height] = window_->GetWindowSize();
    cmd.dispatch((width + 15) / 16, (height + 15) / 16, 1);
  }
  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eGeneral,
                                     vk::ImageLayout::eColorAttachmentOptimal);

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());
  // -----------------------------------------------------------
  // Graphics pass - render debug lines
  // -----------------------------------------------------------
  ZoneNamedN(graphicslineszone, "DebugLinesPass", true);

  constexpr vk::DebugUtilsLabelEXT label_info3{.pLabelName = "DebugLinesPass"};
  cmd.beginDebugUtilsLabelEXT(label_info3, instance_->getDynamicLoader());

  const vk::RenderingAttachmentInfo debug_line_color_attachment{
      .imageView = frame->RenderImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
  };

  const vk::RenderingInfo debug_line_render_info{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &debug_line_color_attachment};

  cmd.beginRendering(debug_line_render_info);
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, debug_line_pipeline_->get());

  cmd.pushConstants(debug_line_pipeline_layout_->get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstant),
                    &push_constant);
  const auto debug_line_vertex_buffer = frame->DebugLineVertexBuffer()->get();
  cmd.bindVertexBuffers(0, 1, &debug_line_vertex_buffer, &offset);

  cmd.setViewport(0, 1, &viewport);

  cmd.setScissor(0, 1, &scissor);

  cmd.draw(debug_line_vertices_.size(), 1, 0, 0);

  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  // -----------------------------------------------------------
  // ImGui pass
  // -----------------------------------------------------------
  ZoneNamedN(imguizone, "ImGui", true);

  constexpr vk::DebugUtilsLabelEXT label_info4{.pLabelName = "ImGuiPass"};
  cmd.beginDebugUtilsLabelEXT(label_info4, instance_->getDynamicLoader());

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("uhh");
  ImGui::End();

  ImGui::Render();

  const vk::RenderingAttachmentInfo imgui_color_attachment{
      .imageView = frame->RenderImage()->view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad, // Load existing scene
      .storeOp = vk::AttachmentStoreOp::eStore,
  };

  const vk::RenderingInfo imgui_render_info{
      .renderArea = vk::Rect2D{.offset = {.x = 0, .y = 0}, .extent = swap_chain_->extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &imgui_color_attachment};

  cmd.beginRendering(imgui_render_info);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  cmd.endRendering();

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  ZoneNamedN(blitzone, "Blitting", true);
  constexpr vk::DebugUtilsLabelEXT label_info5{.pLabelName = "BlittingPass"};
  cmd.beginDebugUtilsLabelEXT(label_info5, instance_->getDynamicLoader());

  VulkanImage::TransitionImageLayout(frame->RenderImage()->get(), cmd, vk::ImageLayout::eColorAttachmentOptimal,
                                     vk::ImageLayout::eTransferSrcOptimal);
  VulkanImage::TransitionImageLayout(swap_chain_->getImage(image_index), cmd, vk::ImageLayout::ePresentSrcKHR,
                                     vk::ImageLayout::eTransferDstOptimal);

  const vk::ImageBlit blit_region{.srcSubresource =
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

  cmd.blitImage(frame->RenderImage()->get(), vk::ImageLayout::eTransferSrcOptimal, swap_chain_->getImage(image_index),
                vk::ImageLayout::eTransferDstOptimal, 1U, &blit_region, vk::Filter::eNearest);

  VulkanImage::TransitionImageLayout(swap_chain_->getImage(image_index), cmd, vk::ImageLayout::eTransferDstOptimal,
                                     vk::ImageLayout::ePresentSrcKHR);

  cmd.endDebugUtilsLabelEXT(instance_->getDynamicLoader());

  cmd.end();

  // -----------------------------------------------------------
  // End frame
  // -----------------------------------------------------------
  ZoneNamedN(endzone, "Endzone", true);
  EndFrame(image_index);
}

uint32_t VulkanRenderer::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                                 uint32_t first_index, int32_t vertex_offset, const glm::vec3& b_min,
                                 const glm::vec3& b_max)
{
  const uint32_t mesh_id = mesh_infos_.size();
  vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());
  indices_.insert(indices_.end(), indices.begin(), indices.end());
  mesh_infos_.emplace_back(b_min, indices.size(), b_max, first_index, vertex_offset);
  return mesh_id;
}

void VulkanRenderer::RenderLine(const glm::vec3& point_a, const glm::vec3& point_b, const glm::vec3& color)
{
  debug_line_vertices_.emplace_back(point_a, color);
  debug_line_vertices_.emplace_back(point_b, color);
}

void VulkanRenderer::ClearLines() { debug_line_vertices_.clear(); }

void VulkanRenderer::Upload()
{
  device_->WaitIdle(); // THIS BAD
  // Destroy old buffers
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

  // Create staging buffers and write the vertices, indices and mesh info to them.
  const auto vertices_size = sizeof(Vertex) * vertices_.size();
  const auto indices_size = sizeof(uint32_t) * indices_.size();
  const auto meshes_size = sizeof(MeshInfo) * mesh_infos_.size();

  vertex_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = vertices_size,
                 .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  index_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = indices_size,
                 .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  mesh_info_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = meshes_size,
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT},
      allocator_->get(), device_.get());

  auto staging_vertex_buffer =
      VulkanBuffer(BufferInfo{.size = vertices_size,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  staging_vertex_buffer.Write(vertices_.data());

  auto staging_index_buffer =
      VulkanBuffer(BufferInfo{.size = indices_size,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  staging_index_buffer.Write(indices_.data());

  auto staging_mesh_info_buffer =
      VulkanBuffer(BufferInfo{.size = meshes_size,
                              .usage = vk::BufferUsageFlagBits::eTransferSrc,
                              .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                              .memoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT},
                   allocator_->get(), device_.get());
  staging_mesh_info_buffer.Write(mesh_infos_.data());

  // Copy staging buffers to real buffers
  const auto cmd = util::BeginSingleTimeCommandBuffer(*transfer_pool_);

  vk::BufferCopy vertex_copy_region = {};
  vertex_copy_region.srcOffset = 0;
  vertex_copy_region.dstOffset = 0;
  vertex_copy_region.size = vertices_size;
  cmd.copyBuffer(staging_vertex_buffer.get(), vertex_buffer_->get(), 1, &vertex_copy_region);

  vk::BufferCopy index_copy_region = {};
  index_copy_region.srcOffset = 0;
  index_copy_region.dstOffset = 0;
  index_copy_region.size = indices_size;
  cmd.copyBuffer(staging_index_buffer.get(), index_buffer_->get(), 1, &index_copy_region);

  vk::BufferCopy mesh_info_copy_region = {};
  mesh_info_copy_region.srcOffset = 0;
  mesh_info_copy_region.dstOffset = 0;
  mesh_info_copy_region.size = meshes_size;
  cmd.copyBuffer(staging_mesh_info_buffer.get(), mesh_info_buffer_->get(), 1, &mesh_info_copy_region);

  util::EndSingleTimeCommandBuffer(cmd, device_->TransferQueue(), *transfer_pool_);

  // Update the mesh info descriptor set
  auto buffer_info = vk::DescriptorBufferInfo{.buffer = mesh_info_buffer_->get(), .offset = 0, .range = vk::WholeSize};

  const vk::WriteDescriptorSet write{.dstSet = static_descriptor_set_,
                                     .dstBinding = 0,
                                     .dstArrayElement = 0,
                                     .descriptorCount = 1,
                                     .descriptorType = vk::DescriptorType::eStorageBuffer,
                                     .pBufferInfo = &buffer_info};
  device_->get().updateDescriptorSets(1, &write, 0, nullptr);
  device_->WaitIdle(); // THIS BAD
}

void VulkanRenderer::RenderMesh(glm::mat4 model, const uint32_t mesh_id)
{
  render_objects_.emplace_back(model, mesh_id);
}

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

  uint32_t image_index{};
  result = swap_chain_->AcquireNextImage(frame->ImageAvailable(), image_index);

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    return std::nullopt;
  }

  device_->get().resetFences(frame->InFlight());

  return image_index;
}

auto VulkanRenderer::EndFrame(const uint32_t image_index) -> void
{
  ZoneScopedN("VulkanRenderer::endFrame");
  const auto& frame = frames_.at(current_frame_);

  const vk::SemaphoreSubmitInfo wait_semaphore{.semaphore = frame->ImageAvailable(),
                                               .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput};

  const vk::SemaphoreSubmitInfo signal_semaphore{.semaphore = submit_semaphores_.at(image_index).get(),
                                                 .stageMask = vk::PipelineStageFlagBits2::eAllCommands};

  const vk::CommandBufferSubmitInfo cmd_info{.commandBuffer = frame->GraphicsCmd()};

  const vk::SubmitInfo2 submit_info{.waitSemaphoreInfoCount = 1,
                                    .pWaitSemaphoreInfos = &wait_semaphore,
                                    .commandBufferInfoCount = 1,
                                    .pCommandBufferInfos = &cmd_info,
                                    .signalSemaphoreInfoCount = 1,
                                    .pSignalSemaphoreInfos = &signal_semaphore};

  auto result = device_->GraphicsQueue().submit2(1, &submit_info, frame->InFlight());
  if (result != vk::Result::eSuccess)
  {
    throw std::runtime_error("submit2 failed: " + vk::to_string(result));
  }

  result = swap_chain_->Present(image_index, device_->PresentQueue(), submit_semaphores_.at(image_index).get());

  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    RecreateSwapChain();
  }

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
}

void VulkanRenderer::RecreateSwapChain()
{
  const auto [width, height] = window_->GetWindowSize();

  swap_chain_->Recreate({.width = width, .height = height});

  RecreateFrameImages(width, height);

  aspect_ratio_ = static_cast<float>(width) / static_cast<float>(height);
}

void VulkanRenderer::RecreateFrameImages(const uint32_t width, const uint32_t height) const
{
  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(frames_.size() * 2);
  std::vector<vk::DescriptorImageInfo> image_infos;
  image_infos.reserve(frames_.size() * 2);
  for (const auto& frame: frames_)
  {
    frame->RecreateFrameImages(width, height);

    image_infos.push_back(vk::DescriptorImageInfo{.sampler = visibility_sampler_.get(),
                                                  .imageView = frame->VisibilityImage()->view(),
                                                  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal});
    const vk::WriteDescriptorSet visibility_write{.dstSet = frame->DescriptorSet(),
                                                  .dstBinding = 3,
                                                  .dstArrayElement = 0,
                                                  .descriptorCount = 1,
                                                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                                  .pImageInfo = &image_infos.back()};
    writes.push_back(visibility_write);


    image_infos.push_back(vk::DescriptorImageInfo{.sampler = VK_NULL_HANDLE,
                                                  .imageView = frame->RenderImage()->view(),
                                                  .imageLayout = vk::ImageLayout::eGeneral});
    const vk::WriteDescriptorSet render_write{.dstSet = frame->DescriptorSet(),
                                              .dstBinding = 4,
                                              .dstArrayElement = 0,
                                              .descriptorCount = 1,
                                              .descriptorType = vk::DescriptorType::eStorageImage,
                                              .pImageInfo = &image_infos.back()};
    writes.push_back(render_write);
  }

  device_->get().updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
}

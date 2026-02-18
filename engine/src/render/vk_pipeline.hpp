#pragma once

#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

struct PipelineLayoutInfo
{
  std::vector<vk::PushConstantRange> push_constants;
  std::vector<vk::DescriptorSetLayout> descriptor_sets;
};

class VulkanPipelineLayout
{
public:
  VulkanPipelineLayout(vk::Device device, const PipelineLayoutInfo &info);
  VulkanPipelineLayout(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout(VulkanPipelineLayout &&) = delete;
  VulkanPipelineLayout &operator=(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout &operator=(VulkanPipelineLayout &&) = delete;
  ~VulkanPipelineLayout();

  [[nodiscard]] vk::PipelineLayout get() const { return layout_; }

private:
  vk::PipelineLayout layout_;
  vk::Device device_;
};

struct ComputePipelineInfo
{
  vk::PipelineShaderStageCreateInfo shader_stage;
  vk::PipelineLayout layout;
};

struct GraphicsPipelineInfo
{
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

  std::vector<vk::VertexInputBindingDescription> vertex_bindings;
  std::vector<vk::VertexInputAttributeDescription> vertex_attributes;

  vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

  vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
  vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
  vk::FrontFace front_face = vk::FrontFace::eCounterClockwise;
  float line_width = 1.0F;

  vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

  bool depth_test = true;
  bool depth_write = true;
  vk::CompareOp depth_compare_op = vk::CompareOp::eLess;

  struct ColorBlendAttachment
  {
    bool blend_enable = false;
    vk::BlendFactor src_color_blend_factor = vk::BlendFactor::eOne;
    vk::BlendFactor dst_color_blend_factor = vk::BlendFactor::eZero;
    vk::BlendOp color_blend_op = vk::BlendOp::eAdd;
    vk::BlendFactor src_alpha_blend_factor = vk::BlendFactor::eOne;
    vk::BlendFactor dst_alpha_blend_factor = vk::BlendFactor::eZero;
    vk::BlendOp alpha_blend_op = vk::BlendOp::eAdd;
    vk::ColorComponentFlags color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  };
  std::vector<ColorBlendAttachment> color_blend_attachments;

  std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

  std::vector<vk::Format> color_attachment_formats;
  vk::Format depth_attachment_format = vk::Format::eUndefined;
  vk::Format stencil_attachment_format = vk::Format::eUndefined;

  vk::PipelineLayout layout;
};

using PipelineInfo = std::variant<GraphicsPipelineInfo, ComputePipelineInfo>;

class VulkanPipeline
{
public:
  VulkanPipeline(vk::Device device, const PipelineInfo &info);
  VulkanPipeline(const VulkanPipeline &) = delete;
  VulkanPipeline(VulkanPipeline &&) = delete;
  VulkanPipeline &operator=(const VulkanPipeline &) = delete;
  VulkanPipeline &operator=(VulkanPipeline &&) = delete;
  ~VulkanPipeline();

  [[nodiscard]] vk::Pipeline get() const { return pipeline_; }

private:
  void CreateGraphicsPipeline(const GraphicsPipelineInfo &info);
  void CreateComputePipeline(const ComputePipelineInfo &info);

  vk::Pipeline pipeline_;
  vk::Device device_;
};

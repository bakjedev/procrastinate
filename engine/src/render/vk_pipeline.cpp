#include "vk_pipeline.hpp"

#include <variant>

#include "vulkan/vulkan.hpp"

VulkanPipelineLayout::VulkanPipelineLayout(const vk::Device device, const PipelineLayoutInfo &info) : device_(device)
{
  const vk::PipelineLayoutCreateInfo create_info{
      .setLayoutCount = static_cast<uint32_t>(info.descriptor_sets.size()),
      .pSetLayouts = info.descriptor_sets.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(info.push_constants.size()),
      .pPushConstantRanges = info.push_constants.data()};

  layout_ = device.createPipelineLayout(create_info);
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
  if (layout_)
  {
    device_.destroyPipelineLayout(layout_);
  }
}

VulkanPipeline::VulkanPipeline(const vk::Device device, const PipelineInfo &info) : device_(device)
{
  std::visit(
      [this](auto &&value)
      {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, GraphicsPipelineInfo>)
        {
          CreateGraphicsPipeline(value);
        } else if constexpr (std::is_same_v<T, ComputePipelineInfo>)
        {
          CreateComputePipeline(value);
        }
      },
      info);
}

VulkanPipeline::~VulkanPipeline()
{
  if (pipeline_)
  {
    device_.destroyPipeline(pipeline_);
  }
}

void VulkanPipeline::CreateGraphicsPipeline(const GraphicsPipelineInfo &info)
{
  vk::PipelineVertexInputStateCreateInfo vertex_input{
      .vertexBindingDescriptionCount = static_cast<uint32_t>(info.vertex_bindings.size()),
      .pVertexBindingDescriptions = info.vertex_bindings.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(info.vertex_attributes.size()),
      .pVertexAttributeDescriptions = info.vertex_attributes.data(),
  };

  vk::PipelineInputAssemblyStateCreateInfo input_assembly{
      .topology = info.topology,
      .primitiveRestartEnable = VK_FALSE,
  };

  vk::PipelineViewportStateCreateInfo viewport{
      .viewportCount = 1, .pViewports = nullptr, .scissorCount = 1, .pScissors = nullptr}; // will be set dynamically

  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = info.polygon_mode,
      .cullMode = info.cull_mode,
      .frontFace = info.front_face,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = info.line_width,
  };

  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = info.samples,
  };

  vk::PipelineDepthStencilStateCreateInfo depth_stencil{
      .depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = info.depth_write ? VK_TRUE : VK_FALSE,
      .depthCompareOp = info.depth_compare_op,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };

  std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
  color_blend_attachments.reserve(info.color_blend_attachments.size());
  for (const auto &attachment: info.color_blend_attachments)
  {
    color_blend_attachments.push_back({
        .blendEnable = attachment.blend_enable ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = attachment.src_color_blend_factor,
        .dstColorBlendFactor = attachment.dst_color_blend_factor,
        .colorBlendOp = attachment.color_blend_op,
        .srcAlphaBlendFactor = attachment.src_alpha_blend_factor,
        .dstAlphaBlendFactor = attachment.dst_alpha_blend_factor,
        .alphaBlendOp = attachment.alpha_blend_op,
        .colorWriteMask = attachment.color_write_mask,
    });
  }

  vk::PipelineColorBlendStateCreateInfo color_blending{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = static_cast<uint32_t>(color_blend_attachments.size()),
      .pAttachments = color_blend_attachments.data(),
      .blendConstants = std::array<float, 4>{0.0F, 0.0F, 0.0F, 0.0F},
  };

  vk::PipelineDynamicStateCreateInfo dynamic_state{
      .dynamicStateCount = static_cast<uint32_t>(info.dynamic_states.size()),
      .pDynamicStates = info.dynamic_states.data(),
  };

  vk::PipelineRenderingCreateInfo rendering_info{
      .viewMask = 0,
      .colorAttachmentCount = static_cast<uint32_t>(info.color_attachment_formats.size()),
      .pColorAttachmentFormats = info.color_attachment_formats.data(),
      .depthAttachmentFormat = info.depth_attachment_format,
      .stencilAttachmentFormat = info.stencil_attachment_format,
  };

  vk::GraphicsPipelineCreateInfo create_info{
      .pNext = &rendering_info,
      .stageCount = static_cast<uint32_t>(info.shader_stages.size()),
      .pStages = info.shader_stages.data(),
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depth_stencil,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state,
      .layout = info.layout,
  };

  auto result = device_.createGraphicsPipelines(nullptr, create_info);
  pipeline_ = result.value.front();
}

void VulkanPipeline::CreateComputePipeline(const ComputePipelineInfo &info)
{
  const vk::ComputePipelineCreateInfo create_info{
      .flags = {},
      .stage = info.shader_stage,
      .layout = info.layout,
  };
  const auto result = device_.createComputePipelines(nullptr, create_info);
  pipeline_ = result.value.front();
}

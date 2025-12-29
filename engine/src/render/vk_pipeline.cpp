#include "vk_pipeline.hpp"

#include <variant>

#include "vulkan/vulkan.hpp"

VulkanPipelineLayout::VulkanPipelineLayout(const vk::Device device,
                                           const PipelineLayoutInfo &info)
    : m_device(device) {
  const vk::PipelineLayoutCreateInfo createInfo{
      .setLayoutCount = static_cast<uint32_t>(info.descriptorSets.size()),
      .pSetLayouts = info.descriptorSets.data(),
      .pushConstantRangeCount =
          static_cast<uint32_t>(info.pushConstants.size()),
      .pPushConstantRanges = info.pushConstants.data()};

  m_layout = device.createPipelineLayout(createInfo);
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
  if (m_layout) {
    m_device.destroyPipelineLayout(m_layout);
  }
}

VulkanPipeline::VulkanPipeline(const vk::Device device,
                               const PipelineInfo &info)
    : m_device(device) {
  std::visit(
      [this](auto &&value) {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, GraphicsPipelineInfo>) {
          createGraphicsPipeline(value);
        } else if constexpr (std::is_same_v<T, ComputePipelineInfo>) {
          createComputePipeline(value);
        }
      },
      info);
}

VulkanPipeline::~VulkanPipeline() {
  if (m_pipeline) {
    m_device.destroyPipeline(m_pipeline);
  }
}

void VulkanPipeline::createGraphicsPipeline(const GraphicsPipelineInfo &info) {
  vk::PipelineVertexInputStateCreateInfo vertexInput{
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(info.vertexBindings.size()),
      .pVertexBindingDescriptions = info.vertexBindings.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(info.vertexAttributes.size()),
      .pVertexAttributeDescriptions = info.vertexAttributes.data(),
  };

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      .topology = info.topology,
      .primitiveRestartEnable = VK_FALSE,
  };

  vk::PipelineViewportStateCreateInfo viewport{
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr};  // will be set dynamically

  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = info.polygonMode,
      .cullMode = info.cullMode,
      .frontFace = info.frontFace,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = info.lineWidth,
  };

  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = info.samples,
  };

  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      .depthTestEnable = info.depthTest ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = info.depthWrite ? VK_TRUE : VK_FALSE,
      .depthCompareOp = info.depthCompareOp,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };

  std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
  colorBlendAttachments.reserve(info.colorBlendAttachments.size());
  for (const auto &attachment : info.colorBlendAttachments) {
    colorBlendAttachments.push_back({
        .blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = attachment.srcColorBlendFactor,
        .dstColorBlendFactor = attachment.dstColorBlendFactor,
        .colorBlendOp = attachment.colorBlendOp,
        .srcAlphaBlendFactor = attachment.srcAlphaBlendFactor,
        .dstAlphaBlendFactor = attachment.dstAlphaBlendFactor,
        .alphaBlendOp = attachment.alphaBlendOp,
        .colorWriteMask = attachment.colorWriteMask,
    });
  }

  vk::PipelineColorBlendStateCreateInfo colorBlending{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
      .pAttachments = colorBlendAttachments.data(),
      .blendConstants = std::array<float, 4>{0.0F, 0.0F, 0.0F, 0.0F},
  };

  vk::PipelineDynamicStateCreateInfo dynamicState{
      .dynamicStateCount = static_cast<uint32_t>(info.dynamicStates.size()),
      .pDynamicStates = info.dynamicStates.data(),
  };

  vk::PipelineRenderingCreateInfo renderingInfo{
      .viewMask = 0,
      .colorAttachmentCount =
          static_cast<uint32_t>(info.colorAttachmentFormats.size()),
      .pColorAttachmentFormats = info.colorAttachmentFormats.data(),
      .depthAttachmentFormat = info.depthAttachmentFormat,
      .stencilAttachmentFormat = info.stencilAttachmentFormat,
  };

  vk::GraphicsPipelineCreateInfo createInfo{
      .pNext = &renderingInfo,
      .stageCount = static_cast<uint32_t>(info.shaderStages.size()),
      .pStages = info.shaderStages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = info.layout,
  };

  auto result = m_device.createGraphicsPipelines(nullptr, createInfo);
  m_pipeline = result.value.front();
}

void VulkanPipeline::createComputePipeline(const ComputePipelineInfo &info) {
  const vk::ComputePipelineCreateInfo createInfo{
      .flags = {},
      .stage = info.shaderStage,
      .layout = info.layout,
  };
  const auto result = m_device.createComputePipelines(nullptr, createInfo);
  m_pipeline = result.value.front();
}
#include "vk_pipeline.hpp"

#include "util/vk_check.hpp"

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device,
                                           const PipelineLayoutInfo &info)
    : m_device(device) {
  VkPipelineLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .setLayoutCount = static_cast<uint32_t>(info.descriptorSets.size()),
      .pSetLayouts = info.descriptorSets.data(),
      .pushConstantRangeCount =
          static_cast<uint32_t>(info.pushConstants.size()),
      .pPushConstantRanges = info.pushConstants.data()};

  vkCreatePipelineLayout(device, &createInfo, nullptr, &m_layout);
}

VulkanPipelineLayout::~VulkanPipelineLayout() {
  if (m_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
  }
}

VulkanPipeline::VulkanPipeline(VkDevice device, VulkanPipelineType type,
                               const PipelineInfo &info)
    : m_device(device) {
  switch (type) {
    case VulkanPipelineType::Graphics:
      createGraphicsPipeline(info);
      break;
    case VulkanPipelineType::Compute:
      createComputePipeline(info);
      break;
  }
}

VulkanPipeline::~VulkanPipeline() {
  if (m_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
  }
}

void VulkanPipeline::createGraphicsPipeline(const PipelineInfo &info) {
  VkPipelineVertexInputStateCreateInfo vertexInput{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .vertexBindingDescriptionCount =
          static_cast<uint32_t>(info.vertexBindings.size()),
      .pVertexBindingDescriptions = info.vertexBindings.data(),
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(info.vertexAttributes.size()),
      .pVertexAttributeDescriptions = info.vertexAttributes.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .topology = info.topology,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkPipelineViewportStateCreateInfo viewport{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr};

  VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = info.polygonMode,
      .cullMode = info.cullMode,
      .frontFace = info.frontFace,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0F,
      .depthBiasClamp = 0.0F,
      .depthBiasSlopeFactor = 0.0F,
      .lineWidth = info.lineWidth,
  };

  VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .rasterizationSamples = info.samples,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0F,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
  };

  VkPipelineDepthStencilStateCreateInfo depthStencil{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .depthTestEnable = info.depthTest ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = info.depthWrite ? VK_TRUE : VK_FALSE,
      .depthCompareOp = info.depthCompareOp,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0.0F,
      .maxDepthBounds = 0.0F,
  };

  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
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

  VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
      .pAttachments = colorBlendAttachments.data(),
      .blendConstants = {0.0F, 0.0F, 0.0F, 0.0F},
  };

  VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .dynamicStateCount = static_cast<uint32_t>(info.dynamicStates.size()),
      .pDynamicStates = info.dynamicStates.data(),
  };

  VkPipelineRenderingCreateInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount =
          static_cast<uint32_t>(info.colorAttachmentFormats.size()),
      .pColorAttachmentFormats = info.colorAttachmentFormats.data(),
      .depthAttachmentFormat = info.depthAttachmentFormat,
      .stencilAttachmentFormat = info.stencilAttachmentFormat,
  };

  VkGraphicsPipelineCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &renderingInfo,
      .flags = 0U,
      .stageCount = static_cast<uint32_t>(info.shaderStages.size()),
      .pStages = info.shaderStages.data(),
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &inputAssembly,
      .pTessellationState = nullptr,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicState,
      .layout = info.layout,
      .renderPass = nullptr,
      .subpass = 0,
      .basePipelineHandle = nullptr,
      .basePipelineIndex = 0,
  };
  VK_CHECK(vkCreateGraphicsPipelines(m_device, nullptr, 1, &createInfo, nullptr,
                                     &m_pipeline));
}
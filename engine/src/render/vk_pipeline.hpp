#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct PipelineLayoutInfo {
  std::vector<vk::PushConstantRange> pushConstants;
  std::vector<vk::DescriptorSetLayout> descriptorSets;
};

class VulkanPipelineLayout {
public:
  VulkanPipelineLayout(vk::Device device, const PipelineLayoutInfo &info);
  VulkanPipelineLayout(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout(VulkanPipelineLayout &&) = delete;
  VulkanPipelineLayout &operator=(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout &operator=(VulkanPipelineLayout &&) = delete;
  ~VulkanPipelineLayout();

  [[nodiscard]] vk::PipelineLayout get() const { return m_layout; }

private:
  vk::PipelineLayout m_layout;
  vk::Device m_device;
};

enum class VulkanPipelineType : uint8_t { Graphics, Compute };

struct PipelineInfo {
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

  std::vector<vk::VertexInputBindingDescription> vertexBindings;
  std::vector<vk::VertexInputAttributeDescription> vertexAttributes;

  vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;

  vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
  vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
  vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
  float lineWidth = 1.0F;

  vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

  bool depthTest = true;
  bool depthWrite = true;
  vk::CompareOp depthCompareOp = vk::CompareOp::eLess;

  struct ColorBlendAttachment {
    bool blendEnable = false;
    vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eOne;
    vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero;
    vk::BlendOp colorBlendOp = vk::BlendOp::eAdd;
    vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne;
    vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero;
    vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd;
    vk::ColorComponentFlags colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  };
  std::vector<ColorBlendAttachment> colorBlendAttachments;

  std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};

  std::vector<vk::Format> colorAttachmentFormats;
  vk::Format depthAttachmentFormat = vk::Format::eUndefined;
  vk::Format stencilAttachmentFormat = vk::Format::eUndefined;

  vk::PipelineLayout layout;
};

class VulkanPipeline {
public:
  VulkanPipeline(vk::Device device, VulkanPipelineType type,
                 const PipelineInfo &info);
  VulkanPipeline(const VulkanPipeline &) = delete;
  VulkanPipeline(VulkanPipeline &&) = delete;
  VulkanPipeline &operator=(const VulkanPipeline &) = delete;
  VulkanPipeline &operator=(VulkanPipeline &&) = delete;
  ~VulkanPipeline();

  [[nodiscard]] vk::Pipeline get() const { return m_pipeline; }

private:
  void createGraphicsPipeline(const PipelineInfo &info);
  void createComputePipeline([[maybe_unused]] const PipelineInfo &info) {
  } // empty for now

  vk::Pipeline m_pipeline;
  vk::Device m_device;
};
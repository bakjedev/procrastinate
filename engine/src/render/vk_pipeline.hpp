#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct PipelineLayoutInfo {
  std::vector<VkPushConstantRange> pushConstants;
  std::vector<VkDescriptorSetLayout> descriptorSets;
};

class VulkanPipelineLayout {
 public:
  VulkanPipelineLayout(VkDevice device, const PipelineLayoutInfo &info);
  VulkanPipelineLayout(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout(VulkanPipelineLayout &&) = delete;
  VulkanPipelineLayout &operator=(const VulkanPipelineLayout &) = delete;
  VulkanPipelineLayout &operator=(VulkanPipelineLayout &&) = delete;
  ~VulkanPipelineLayout();

  [[nodiscard]] VkPipelineLayout get() const { return m_layout; }

 private:
  VkPipelineLayout m_layout = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
};

enum class VulkanPipelineType : uint8_t { Graphics, Compute };

struct PipelineInfo {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

  std::vector<VkVertexInputBindingDescription> vertexBindings;
  std::vector<VkVertexInputAttributeDescription> vertexAttributes;

  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
  VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  float lineWidth = 1.0F;

  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

  bool depthTest = true;
  bool depthWrite = true;
  VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

  struct ColorBlendAttachment {
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  };
  std::vector<ColorBlendAttachment> colorBlendAttachments;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  std::vector<VkFormat> colorAttachmentFormats;
  VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
  VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkPipelineLayout layout = VK_NULL_HANDLE;
};

class VulkanPipeline {
 public:
  VulkanPipeline(VkDevice device, VulkanPipelineType type,
                 const PipelineInfo &info);
  VulkanPipeline(const VulkanPipeline &) = delete;
  VulkanPipeline(VulkanPipeline &&) = delete;
  VulkanPipeline &operator=(const VulkanPipeline &) = delete;
  VulkanPipeline &operator=(VulkanPipeline &&) = delete;
  ~VulkanPipeline();

  [[nodiscard]] VkPipeline get() const { return m_pipeline; }

 private:
  void createGraphicsPipeline(const PipelineInfo &info);
  void createComputePipeline([[maybe_unused]] const PipelineInfo &info) {
  }  // empty for now

  VkPipeline m_pipeline = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
};
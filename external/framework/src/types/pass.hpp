#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vulkan/vulkan.h>
#include "resource.hpp"

namespace fwrk {
  struct ClearValue {
    float r{};
    float g{};
    float b{};
    float a{};
  };

  struct Attachment {
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ClearValue clear_value;
    std::optional<uint32_t> resolve = std::nullopt;
    VkResolveModeFlags resolve_mode = VK_RESOLVE_MODE_NONE;
  };

  struct ImageAccess {
    ResourceID resource;
    std::optional<Attachment> attachment;
    VkImageAspectFlags aspect{};
    uint32_t base_level{};
    uint32_t level_count{};
    uint32_t base_layer{};
    uint32_t layer_count{};
    VkImageViewType view_type{};

    VkAccessFlags2 access{};
    VkPipelineStageFlags2 stages{};
    VkImageLayout layout{};
  };

  struct BufferAccess {
    ResourceID resource;
    VkDeviceSize size = VK_WHOLE_SIZE;
    VkDeviceSize offset = 0;

    VkAccessFlags2 access = VK_ACCESS_2_NONE;
    VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
  };

  struct RenderInfo {
    std::optional<VkExtent2D> render_area;
    uint32_t layer_count = 1;
    uint32_t view_mask = 0;
  };

  struct Pass {
    std::string name;
    std::function<void(VkCommandBuffer cmd)> func;
    std::vector<ImageAccess> images;
    std::vector<BufferAccess> buffers;
    RenderInfo render_info;
  };
} // namespace fwrk

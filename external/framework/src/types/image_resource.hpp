#pragma once
#include <vulkan/vulkan.h>

namespace fwrk {
  struct ImageState {
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;

    static const ImageState Undefined;

    bool operator==(const ImageState& other) const = default;
  };
  constexpr ImageState ImageState::Undefined{VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED};

  struct ImageResource {
    VkImageType type;
    VkExtent3D size;
    VkFormat format;
    VkImageUsageFlags usage;

    ImageState state;
  };

  template<typename T>
  concept ImageInterface = requires(const T& img) {
    { img.type() } -> std::same_as<VkImageType>;
    { img.size() } -> std::same_as<VkExtent3D>;
    { img.format() } -> std::same_as<VkFormat>;
    { img.usage() } -> std::same_as<VkImageUsageFlags>;
    { img.image() } -> std::same_as<VkImage>;
  };
} // namespace fwrk

#pragma once
#include <vulkan/vulkan.h>

namespace fwrk {
  struct BufferState {
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;

    static const BufferState Undefined;

    bool operator==(const BufferState& other) const = default;
  };
  constexpr BufferState BufferState::Undefined{VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE};

  struct BufferResource {
    VkDeviceSize size;
    VkBufferUsageFlags usage;

    BufferState state;
  };

  template<typename T>
  concept BufferInterface = requires(const T& buf) {
    { buf.size() } -> std::same_as<VkDeviceSize>;
    { buf.usage() } -> std::same_as<VkBufferUsageFlags>;
    { buf.buffer() } -> std::same_as<VkBuffer>;
  };
} // namespace fwrk

#pragma once

#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "render/vk_command_pool.hpp"

namespace util
{
  inline vk::CommandBuffer BeginSingleTimeCommandBuffer(const VulkanCommandPool& pool)
  {
    const auto command_buffer = pool.allocate();

    constexpr vk::CommandBufferBeginInfo info{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    command_buffer.begin(info);
    return command_buffer;
  }

  inline void EndSingleTimeCommandBuffer(const vk::CommandBuffer cmd, const vk::Queue queue,
                                         const VulkanCommandPool& pool)
  {
    cmd.end();

    const vk::CommandBufferSubmitInfo cmd_info{.commandBuffer = cmd};

    const vk::SubmitInfo2 submit_info{
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_info,
    };

    auto result = queue.submit2(1, &submit_info, nullptr);
    if (result != vk::Result::eSuccess)
    {
      throw std::runtime_error("Failed to submit to transfer queue");
    }
    queue.waitIdle();

    pool.free(cmd);
  }
} // namespace util

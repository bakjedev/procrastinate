#pragma once

#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "render/vk_command_pool.hpp"

namespace Util {
inline vk::CommandBuffer beginSingleTimeCommandBuffer(
    const VulkanCommandPool& pool) {
  auto commandBuffer = pool.allocate();

  vk::CommandBufferBeginInfo info{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

  commandBuffer.begin(info);
  return commandBuffer;
}

inline void endSingleTimeCommandBuffer(vk::CommandBuffer cmd,
                                       vk::Queue transferQueue,
                                       const VulkanCommandPool& pool) {
  cmd.end();

  vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd};

  vk::SubmitInfo2 submitInfo{
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos = &cmdInfo,
  };

  auto result = transferQueue.submit2(1, &submitInfo, nullptr);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to submit to transfer queue");
  }
  transferQueue.waitIdle();

  pool.free(cmd);
}
}  // namespace Util
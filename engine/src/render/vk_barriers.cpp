#include "render/vk_barriers.hpp"

#include "vulkan/vulkan.hpp"

void VulkanBarriers::bufferBarrier(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit oldUsage,
                                   BufferUsageBit newUsage)
{
  vk::BufferMemoryBarrier2 barrier{};

  const auto source = bufferUsage(oldUsage);
  const auto destination = bufferUsage(newUsage);

  barrier.srcStageMask = source.stage, barrier.srcAccessMask = source.access, barrier.dstStageMask = destination.stage,
  barrier.dstAccessMask = destination.access, barrier.buffer = buffer.buffer, barrier.offset = buffer.offset,
  barrier.size = buffer.size;

  const vk::DependencyInfo dependency{.bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier};

  cmd.pipelineBarrier2(dependency);
}

void VulkanBarriers::bufferBarrierRelease(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit oldUsage,
                                          uint32_t srcFamily, uint32_t dstFamily)
{
  vk::BufferMemoryBarrier2 barrier{};

  const auto source = bufferUsage(oldUsage);

  barrier.srcStageMask = source.stage, barrier.srcAccessMask = source.access,
  barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone, barrier.dstAccessMask = vk::AccessFlagBits2::eNone,
  barrier.buffer = buffer.buffer, barrier.offset = buffer.offset, barrier.size = buffer.size;
  barrier.srcQueueFamilyIndex = srcFamily;
  barrier.dstQueueFamilyIndex = dstFamily;

  const vk::DependencyInfo dependency{.bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier};

  cmd.pipelineBarrier2(dependency);
}

void VulkanBarriers::bufferBarrierAcquire(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit newUsage,
                                          uint32_t srcFamily, uint32_t dstFamily)
{
  vk::BufferMemoryBarrier2 barrier{};

  const auto destination = bufferUsage(newUsage);

  barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone, barrier.srcAccessMask = vk::AccessFlagBits2::eNone,
  barrier.dstStageMask = destination.stage, barrier.dstAccessMask = destination.access, barrier.buffer = buffer.buffer,
  barrier.offset = buffer.offset, barrier.size = buffer.size;
  barrier.srcQueueFamilyIndex = srcFamily;
  barrier.dstQueueFamilyIndex = dstFamily;

  const vk::DependencyInfo dependency{.bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier};

  cmd.pipelineBarrier2(dependency);
}

VulkanBarriers::StageAccess VulkanBarriers::bufferUsage(BufferUsageBit usage)
{
  vk::PipelineStageFlags2 stage{};
  vk::AccessFlags2 access{};

  if ((usage & BufferUsageBit::RGeometry) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eVertexShader;
    stage |= vk::PipelineStageFlagBits2::eTessellationControlShader;
    stage |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
    stage |= vk::PipelineStageFlagBits2::eGeometryShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
  }

  if ((usage & BufferUsageBit::RFragment) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eFragmentShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
  }

  if ((usage & BufferUsageBit::RCompute) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eComputeShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
  }

  if ((usage & BufferUsageBit::RWGeometry) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eVertexShader;
    stage |= vk::PipelineStageFlagBits2::eTessellationControlShader;
    stage |= vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
    stage |= vk::PipelineStageFlagBits2::eGeometryShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
    access |= vk::AccessFlagBits2::eShaderStorageWrite;
  }

  if ((usage & BufferUsageBit::RWFragment) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eFragmentShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
    access |= vk::AccessFlagBits2::eShaderStorageWrite;
  }

  if ((usage & BufferUsageBit::RWCompute) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eComputeShader;

    access |= vk::AccessFlagBits2::eShaderStorageRead;
    access |= vk::AccessFlagBits2::eShaderStorageWrite;
  }

  if ((usage & BufferUsageBit::VertexOrIndex) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eVertexAttributeInput;
    stage |= vk::PipelineStageFlagBits2::eIndexInput;

    access |= vk::AccessFlagBits2::eVertexAttributeRead;
    access |= vk::AccessFlagBits2::eIndexRead;
  }

  if ((usage & BufferUsageBit::IndirectDraw) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eDrawIndirect;

    access |= vk::AccessFlagBits2::eIndirectCommandRead;
  }

  if ((usage & BufferUsageBit::CopySource) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eCopy;

    access |= vk::AccessFlagBits2::eTransferRead;
  }

  if ((usage & BufferUsageBit::CopyDestination) != BufferUsageBit::None)
  {
    stage |= vk::PipelineStageFlagBits2::eCopy;
    stage |= vk::PipelineStageFlagBits2::eTransfer;
    stage |= vk::PipelineStageFlagBits2::eClear;

    access |= vk::AccessFlagBits2::eTransferWrite;
  }

  return {.stage = stage, .access = access};
}

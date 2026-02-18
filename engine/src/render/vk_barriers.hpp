#pragma once

#include <cstdint>
#include <type_traits>
#include <vulkan/vulkan.hpp>

#define ENABLE_BITMASK(E)                                             \
  inline E operator|(E lhs, E rhs)                                    \
  {                                                                   \
    using T = std::underlying_type_t<E>;                              \
    return static_cast<E>(static_cast<T>(lhs) | static_cast<T>(rhs)); \
  }                                                                   \
  inline E operator&(E lhs, E rhs)                                    \
  {                                                                   \
    using T = std::underlying_type_t<E>;                              \
    return static_cast<E>(static_cast<T>(lhs) & static_cast<T>(rhs)); \
  }                                                                   \
  inline E& operator!=(E& lhs, E rhs) { return lhs = lhs | rhs; }     \
  inline E& operator&=(E& lhs, E rhs) { return lhs = lhs & rhs; }

namespace vulkan_barriers
{
  // https://anki3d.org/simplified-pipeline-barriers/
  using UsageType = uint16_t;
  enum class BufferUsageBit : UsageType
  {
    None = 0,

    RGeometry = 1 << 0,
    RFragment = 1 << 1,
    RCompute = 1 << 2,

    RWGeometry = 1 << 3,
    RWFragment = 1 << 4,
    RWCompute = 1 << 5,

    VertexOrIndex = 1 << 6,

    IndirectDraw = 1 << 7,

    CopySource = 1 << 8,
    CopyDestination = 1 << 9,

    // Derived
    AllR = RGeometry | RFragment | RCompute,
    AllRW = RWGeometry | RWFragment | RWCompute,
    AllIndirect = IndirectDraw,
    AllCopy = CopySource | CopyDestination,

    AllGeometry = RGeometry | RWGeometry | VertexOrIndex,
    AllFragment = RFragment | RWFragment,
    AllGraphics = AllGeometry | AllFragment | IndirectDraw,
    AllCompute = RCompute | RWCompute,

    AllRead = AllR | AllRW | VertexOrIndex | AllIndirect | CopySource,
    AllWrite = AllRW | CopyDestination,

    AllShaderResource = AllR | AllRW,

    All = AllRead | AllWrite,
  };

  ENABLE_BITMASK(BufferUsageBit)

  struct BufferInfo
  {
    vk::Buffer buffer;
    size_t size;
    size_t offset{};
  };

  struct StageAccess
  {
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
  };

  void BufferBarrier(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit old_usage,
                     BufferUsageBit new_usage);

  void BufferBarrierRelease(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit old_usage,
                            uint32_t src_family, uint32_t dst_family);

  void BufferBarrierAcquire(vk::CommandBuffer cmd, const BufferInfo& buffer, BufferUsageBit new_usage,
                            uint32_t src_family, uint32_t dst_family);

  StageAccess BufferUsage(BufferUsageBit usage);
} // namespace vulkan_barriers

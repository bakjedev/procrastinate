#pragma once
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "pass_builder.hpp"
#include "types/buffer_resource.hpp"
#include "types/image_resource.hpp"
#include "types/pass.hpp"
#include "types/resource.hpp"

namespace fwrk {
  class Context;

  class Graph {
  public:
    void set_image_end_state(ResourceID resource, const ImageState& state);
    void set_buffer_end_state(ResourceID resource, const BufferState& state);

    [[nodiscard]] GraphicsPassBuilder add_graphics_pass(std::string name = "Unnamed graphics pass");
    [[nodiscard]] ComputePassBuilder add_compute_pass(std::string name = "Unnamed compute pass");

    bool compile();

    void execute(VkCommandBuffer cmd);

  private:
    friend Context;
    template<typename T>
    friend class PassBuilder;
    friend class GraphicsPassBuilder;

    explicit Graph(Context* context) : context_(context) {}

    struct AttachmentResolveInfo {
      ResourceID resource;
      VkResolveModeFlags mode;
      VkImageSubresourceRange subresource;
    };

    struct RenderingAttachmentInfo {
      ResourceID resource;
      VkImageSubresourceRange subresource;
      VkImageViewType view_type;
      VkImageLayout layout;
      VkAttachmentLoadOp load_op;
      VkAttachmentStoreOp store_op;
      VkClearValue clear_value;
      std::optional<AttachmentResolveInfo> resolve;
    };

    struct RenderingInfo {
      std::vector<RenderingAttachmentInfo> color_atts;
      std::optional<RenderingAttachmentInfo> depth_att;
      RenderInfo render_info;
    };

    struct ImageMemoryBarrier {
      ResourceID resource;
      ImageState dst_state{};
      VkImageSubresourceRange subresource_range{};
    };

    struct BufferMemoryBarrier {
      ResourceID resource;
      BufferState dst_state{};
      VkDeviceSize size{};
      VkDeviceSize offset{};
    };

    struct DependencyInfo {
      std::vector<ImageMemoryBarrier> image_barriers;
      std::vector<BufferMemoryBarrier> buffer_barriers;
    };

    struct CompiledPass {
      DependencyInfo deps;
      std::optional<RenderingInfo> render;
      std::string name;
      std::function<void(VkCommandBuffer cmd)> func;
    };

    flat_hash_map<ResourceID, ResourceDependencies> resource_deps_;

    std::vector<Pass> passes_;

    std::vector<uint32_t> sorted_pass_ids_;
    std::vector<CompiledPass> compiled_passes_;

    std::vector<std::pair<ResourceID, ImageState>> end_image_states_;
    std::vector<std::pair<ResourceID, BufferState>> end_buffer_states_;
    DependencyInfo end_dep_info_;

    Context* context_;

    static VkImageAspectFlags get_aspect_for_format(VkFormat format);
  };
} // namespace fwrk

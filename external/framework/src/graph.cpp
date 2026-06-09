#include "graph.hpp"
#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include "context.hpp"
#include "types/pass.hpp"

void fwrk::Graph::set_image_end_state(const ResourceID resource, const ImageState& state)
{
  end_image_states_.emplace_back(resource, state);
}

void fwrk::Graph::set_buffer_end_state(const ResourceID resource, const BufferState& state)
{
  end_buffer_states_.emplace_back(resource, state);
}

fwrk::GraphicsPassBuilder fwrk::Graph::add_graphics_pass(std::string name)
{
  const auto id = passes_.size();
  passes_.emplace_back().name = std::move(name);
  return GraphicsPassBuilder{&passes_.back(), this, id};
}

fwrk::ComputePassBuilder fwrk::Graph::add_compute_pass(std::string name)
{
  const auto id = passes_.size();
  passes_.emplace_back().name = std::move(name);
  return ComputePassBuilder{&passes_.back(), this, id};
}

bool fwrk::Graph::compile()
{
  // -------------------
  // Reset compiled data
  // -------------------
  sorted_pass_ids_.clear();
  end_dep_info_ = {};
  compiled_passes_.clear();

  size_t pass_count = passes_.size();

  // --------------
  // you like DAGs?
  // --------------
  std::vector<std::vector<uint32_t>> incoming{pass_count};
  std::vector<std::vector<uint32_t>> outcoming{pass_count};

  auto add_edge = [&incoming, &outcoming](const uint32_t from, const uint32_t to) {
    outcoming[from].push_back(to);
    incoming[to].push_back(from);
  };

  for (auto& [_, info]: resource_deps_) {
    std::ranges::sort(info.write_passes);
    std::ranges::sort(info.read_passes);
    const auto write_count = static_cast<uint32_t>(info.write_passes.size());
    const auto read_count = static_cast<uint32_t>(info.read_passes.size());

    std::optional<uint32_t> previous_write;
    std::vector<uint32_t> previous_readers;

    uint32_t write_idx = 0;
    uint32_t read_idx = 0;
    while (write_idx < write_count || read_idx < read_count) {
      // if we still have a write and a read and they happen in the same pass
      if (write_idx < write_count && read_idx < read_count &&
          info.write_passes[write_idx] == info.read_passes[read_idx]) {
        uint32_t write_pass = info.write_passes[write_idx];
        if (previous_write) {
          add_edge(*previous_write, write_pass);
        }
        for (uint32_t read: previous_readers) {
          add_edge(read, write_pass);
        }
        previous_write = write_pass;
        previous_readers.clear();
        write_idx++;
        read_idx++;
        // if we still have a write and either we have no more reads or the write comes earlier than the read
      } else if (write_idx < write_count &&
                 (read_idx >= read_count || info.write_passes[write_idx] < info.read_passes[read_idx])) {
        uint32_t write_pass = info.write_passes[write_idx];
        if (previous_write) {
          add_edge(*previous_write, write_pass);
        }
        for (uint32_t read: previous_readers) {
          add_edge(read, write_pass);
        }
        previous_write = write_pass;
        previous_readers.clear();
        write_idx++;
        // if we still have a read but no more writes
      } else {
        uint32_t read_pass = info.read_passes[read_idx];

        auto it = info.read_deps.find(read_pass);
        if (it != info.read_deps.end()) {
          uint32_t other = it->second;
          assert(std::ranges::binary_search(info.write_passes, other));
          add_edge(other, read_pass);
          for (uint32_t write: info.write_passes) {
            if (write > other && write < read_pass) { // loop over all other writers between this read and the explicit
                                                      // dep write and add a reverse edge
              add_edge(read_pass, write);
            }
          }
        } else if (previous_write) {
          add_edge(*previous_write, read_pass);
        }

        previous_readers.push_back(read_pass);
        read_idx++;
      }
    }
  }
  for (auto& in: incoming) {
    std::ranges::sort(in);
    auto [first, last] = std::ranges::unique(in);
    in.erase(first, last);
  }
  for (auto& out: outcoming) {
    std::ranges::sort(out);
    auto [first, last] = std::ranges::unique(out);
    out.erase(first, last);
  }

  // ----------------
  // Topological sort
  // ----------------
  {
    sorted_pass_ids_.reserve(pass_count);

    std::vector<uint32_t> in_deg(pass_count);
    for (uint32_t i = 0; i < pass_count; i++) {
      in_deg[i] = static_cast<uint32_t>(incoming[i].size());
    }

    // find all nodes with no incoming edges
    std::deque<uint32_t> root_nodes;
    for (uint32_t i = 0; i < incoming.size(); i++) {
      if (incoming[i].empty()) {
        root_nodes.push_back(i);
      }
    }

    // sort
    while (!root_nodes.empty()) {
      auto node = root_nodes.front();
      root_nodes.pop_front();
      sorted_pass_ids_.push_back(node);

      for (auto it: outcoming[node]) {
        in_deg[it]--;

        // recurse
        if (in_deg[it] == 0) {
          root_nodes.push_back(it);
        }
      }
    }
  }

  assert(sorted_pass_ids_.size() == passes_.size() && "You created a cycle");

  // ----------------------------------------
  // Create pass barriers and rendering infos
  // ----------------------------------------
  compiled_passes_.resize(pass_count);

  for (const uint32_t pass_id: sorted_pass_ids_) {
    Pass& pass = passes_[pass_id];
    auto& [dependencies, rendering, name, func] = compiled_passes_[pass_id];
    auto& [image_barriers, buffer_barriers] = dependencies;

    name = std::move(pass.name);
    func = std::move(pass.func);

    // image memory barriers
    for (const ImageAccess& image_access: pass.images) {
      const Resource& resource = context_->resources_.at(*image_access.resource.id); // sorta unsafe

      VkImageAspectFlags aspect = image_access.aspect;
      if (aspect == VK_IMAGE_ASPECT_NONE && resource.type == ResourceType::Image) {
        const ImageResource& image = context_->images_.at(resource.slot);
        aspect = get_aspect_for_format(image.format);
      }

      VkImageSubresourceRange subresource{.aspectMask = aspect,
                                          .baseMipLevel = image_access.base_level,
                                          .levelCount = image_access.level_count,
                                          .baseArrayLayer = image_access.base_layer,
                                          .layerCount = image_access.layer_count};

      image_barriers.emplace_back(
          image_access.resource,
          ImageState{.access = image_access.access, .stages = image_access.stages, .layout = image_access.layout},
          subresource);

      // rendering attachment
      if (image_access.attachment) {
        if (!rendering.has_value()) {
          rendering.emplace();
          rendering->render_info = pass.render_info;
        }
        const auto& [load_op, store_op, clear_value, resolve, resolve_mode] = *image_access.attachment;

        RenderingAttachmentInfo info;
        info.resource = image_access.resource;
        info.subresource = subresource;
        info.view_type = image_access.view_type;
        info.layout = image_access.layout;
        info.load_op = load_op;
        info.store_op = store_op;
        if (resolve) {
          const ImageAccess& resolve_access = pass.images.at(*resolve);
          info.resolve->resource = resolve_access.resource;
          info.resolve->subresource.aspectMask = resolve_access.aspect;
          info.resolve->subresource.baseArrayLayer = resolve_access.base_layer;
          info.resolve->subresource.baseMipLevel = resolve_access.base_level;
          info.resolve->subresource.layerCount = resolve_access.layer_count;
          info.resolve->subresource.levelCount = resolve_access.level_count;
          info.resolve->mode = resolve_mode;
        }
        if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
          info.clear_value =
              VkClearValue{.color = {.float32 = {clear_value.r, clear_value.g, clear_value.b, clear_value.a}}};
          rendering->color_atts.push_back(info);
        } else if ((aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0 || (aspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
          info.clear_value =
              VkClearValue{.depthStencil = {.depth = clear_value.r, .stencil = static_cast<uint32_t>(clear_value.r)}};
          rendering->depth_att = info;
        }
      }
    }

    // buffer memory barriers
    for (const BufferAccess& buffer_access: pass.buffers) {
      buffer_barriers.emplace_back(buffer_access.resource,
                                   BufferState{.access = buffer_access.access, .stages = buffer_access.stages},
                                   buffer_access.size, buffer_access.offset);
    }
  }

  return true;
}

void fwrk::Graph::execute(VkCommandBuffer cmd)
{
  if (!cmd) return;
  for (const CompiledPass& pass: compiled_passes_) {
    // ---------------------
    // Create image barriers
    // ---------------------
    std::vector<VkImageMemoryBarrier2> image_barriers;
    for (const auto& img_barr: pass.deps.image_barriers) {
      const Resource* resource = &context_->resources_.at(*img_barr.resource.id); // sorta unsafe
      if (resource->type == ResourceType::Proxy) {
        assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
        resource = &context_->resources_.at(resource->target); // sorta unsafe
      }
      ImageResource& image = context_->images_.at(resource->slot);

      if (image.state != img_barr.dst_state) {
        VkImageAspectFlags aspect = img_barr.subresource_range.aspectMask;
        if (aspect == VK_IMAGE_ASPECT_NONE) {
          aspect = get_aspect_for_format(image.format);
        }

        VkImageMemoryBarrier2& barrier = image_barriers.emplace_back(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
        barrier.srcStageMask = image.state.stages;
        barrier.srcAccessMask = image.state.access;
        barrier.oldLayout = image.state.layout;
        barrier.dstStageMask = img_barr.dst_state.stages;
        barrier.dstAccessMask = img_barr.dst_state.access;
        barrier.newLayout = img_barr.dst_state.layout;
        barrier.image = context_->raw_images_.at(resource->raw);
        barrier.subresourceRange = {.aspectMask = aspect,
                                    .baseMipLevel = img_barr.subresource_range.baseMipLevel,
                                    .levelCount = img_barr.subresource_range.levelCount,
                                    .baseArrayLayer = img_barr.subresource_range.baseArrayLayer,
                                    .layerCount = img_barr.subresource_range.layerCount};

        image.state = img_barr.dst_state;
      }
    }

    // ----------------------
    // Create buffer barriers
    // ----------------------
    std::vector<VkBufferMemoryBarrier2> buffer_barriers;
    for (const auto& buf_barr: pass.deps.buffer_barriers) {
      const Resource* resource = &context_->resources_.at(*buf_barr.resource.id); // sorta unsafe
      if (resource->type == ResourceType::Proxy) {
        assert((resource->target != UINT32_MAX) && "Received a proxy that targets nothing");
        resource = &context_->resources_.at(resource->target); // sorta unsafe
      }
      BufferResource& buffer = context_->buffers_.at(resource->slot);

      if (buffer.state != buf_barr.dst_state) {
        VkBufferMemoryBarrier2& barrier = buffer_barriers.emplace_back(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2);
        barrier.srcStageMask = buffer.state.stages;
        barrier.srcAccessMask = buffer.state.access;
        barrier.dstStageMask = buf_barr.dst_state.stages;
        barrier.dstAccessMask = buf_barr.dst_state.access;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = context_->raw_buffers_.at(resource->raw);
        barrier.offset = buf_barr.offset;
        barrier.size = buf_barr.size;

        buffer.state = buf_barr.dst_state;
      }
    }

    // ----------------------
    // Create dependency info
    // ----------------------
    VkDependencyInfo dep_info{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                              .pNext = nullptr,
                              .dependencyFlags = 0u,
                              .memoryBarrierCount = 0,
                              .pMemoryBarriers = nullptr,
                              .bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size()),
                              .pBufferMemoryBarriers = buffer_barriers.data(),
                              .imageMemoryBarrierCount = static_cast<uint32_t>(image_barriers.size()),
                              .pImageMemoryBarriers = image_barriers.data()};

    // ---------------------
    // Create rendering info
    // ---------------------
    VkRenderingInfo rendering_info{};
    std::vector<VkRenderingAttachmentInfo> color_attachments;
    std::optional<VkRenderingAttachmentInfo> depth_attachment{};
    if (pass.render) {
      VkExtent2D extent{UINT32_MAX, UINT32_MAX};

      for (auto& att: pass.render->color_atts) {
        const Resource* resource = &context_->resources_.at(*att.resource.id); // sorta unsafe
        if (resource->type == ResourceType::Proxy) {
          assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
          resource = &context_->resources_.at(resource->target); // sorta unsafe
        }
        ImageResource& image = context_->images_.at(resource->slot);
        VkRenderingAttachmentInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.imageView = context_->get_image_view({att.subresource, att.view_type}, *resource);
        info.imageLayout = att.layout;
        if (att.resolve) {
          const Resource* resolve_resource = &context_->resources_.at(*att.resolve->resource.id); // sorta unsafe
          if (resolve_resource->type == ResourceType::Proxy) {
            assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
            resolve_resource = &context_->resources_.at(resolve_resource->target); // sorta unsafe
          }
          info.resolveImageView =
              context_->get_image_view({att.resolve->subresource, att.view_type}, *resolve_resource);
          info.resolveMode = static_cast<VkResolveModeFlagBits>(att.resolve->mode);
          info.resolveImageLayout = att.layout;
        }
        info.loadOp = att.load_op;
        info.storeOp = att.store_op;
        info.clearValue = att.clear_value;

        color_attachments.push_back(info);

        extent.width = std::min(extent.width, std::max(1u, image.size.width >> att.subresource.baseMipLevel));
        extent.height = std::min(extent.height, std::max(1u, image.size.height >> att.subresource.baseMipLevel));
      }

      if (pass.render->depth_att) {
        const RenderingAttachmentInfo& att = *pass.render->depth_att;
        const Resource* resource = &context_->resources_.at(*att.resource.id); // sorta unsafe
        if (resource->type == ResourceType::Proxy) {
          assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
          resource = &context_->resources_.at(resource->target); // sorta unsafe
        }
        ImageResource& image = context_->images_.at(resource->slot);

        VkRenderingAttachmentInfo& info = depth_attachment.emplace();
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.imageView = context_->get_image_view({att.subresource, att.view_type}, *resource);
        info.imageLayout = att.layout;
        if (att.resolve) {
          const Resource* resolve_resource = &context_->resources_.at(*att.resolve->resource.id); // sorta unsafe
          if (resolve_resource->type == ResourceType::Proxy) {
            assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
            resolve_resource = &context_->resources_.at(resolve_resource->target); // sorta unsafe
          }
          info.resolveImageView =
              context_->get_image_view({att.resolve->subresource, att.view_type}, *resolve_resource);
          info.resolveMode = static_cast<VkResolveModeFlagBits>(att.resolve->mode);
          info.resolveImageLayout = att.layout;
        }
        info.loadOp = att.load_op;
        info.storeOp = att.store_op;
        info.clearValue = att.clear_value;

        extent.width = std::min(extent.width, std::max(1u, image.size.width >> att.subresource.baseMipLevel));
        extent.height = std::min(extent.height, std::max(1u, image.size.height >> att.subresource.baseMipLevel));
      }
      rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
      rendering_info.layerCount = pass.render->render_info.layer_count;
      rendering_info.viewMask = pass.render->render_info.view_mask;
      rendering_info.renderArea.extent = pass.render->render_info.render_area.value_or(extent);
      rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
      rendering_info.pColorAttachments = color_attachments.data();
      if (depth_attachment) rendering_info.pDepthAttachment = &*depth_attachment;
    }


    // ------------
    // Execute pass
    // ------------
    vkCmdPipelineBarrier2(cmd, &dep_info);
    if (pass.render) vkCmdBeginRendering(cmd, &rendering_info);
    pass.func(cmd);
    if (pass.render) vkCmdEndRendering(cmd);
  }
  // --------------------------
  // Create end image barriers
  // --------------------------
  std::vector<VkImageMemoryBarrier2> end_image_barriers;
  for (const auto& [id, state]: end_image_states_) {
    const Resource* resource = &context_->resources_.at(*id.id);
    if (resource->type == ResourceType::Proxy) {
      assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
      resource = &context_->resources_.at(resource->target); // sorta unsafe
    }
    ImageResource& image = context_->images_.at(resource->slot);

    if (image.state != state) {
      VkImageMemoryBarrier2& barrier = end_image_barriers.emplace_back(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
      barrier.srcStageMask = image.state.stages;
      barrier.srcAccessMask = image.state.access;
      barrier.oldLayout = image.state.layout;
      barrier.dstStageMask = state.stages;
      barrier.dstAccessMask = state.access;
      barrier.newLayout = state.layout;
      barrier.image = context_->raw_images_.at(resource->raw);
      barrier.subresourceRange = {.aspectMask = get_aspect_for_format(image.format),
                                  .baseMipLevel = 0,
                                  .levelCount = VK_REMAINING_MIP_LEVELS,
                                  .baseArrayLayer = 0,
                                  .layerCount = VK_REMAINING_ARRAY_LAYERS};

      image.state = state;
    }
  }

  // --------------------------
  // Create end buffer barriers
  // --------------------------
  std::vector<VkBufferMemoryBarrier2> end_buffer_barriers;
  for (const auto& [id, state]: end_buffer_states_) {
    const Resource* resource = &context_->resources_.at(*id.id);
    if (resource->type == ResourceType::Proxy) {
      assert(resource->target != UINT32_MAX && "Received a proxy that targets nothing");
      resource = &context_->resources_.at(resource->target); // sorta unsafe
    }
    BufferResource& buffer = context_->buffers_.at(resource->slot);

    if (buffer.state != state) {
      VkBufferMemoryBarrier2& barrier = end_buffer_barriers.emplace_back(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2);
      barrier.srcStageMask = buffer.state.stages;
      barrier.srcAccessMask = buffer.state.access;
      barrier.dstStageMask = state.stages;
      barrier.dstAccessMask = state.access;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.buffer = context_->raw_buffers_.at(resource->raw);
      barrier.offset = 0;
      barrier.size = VK_WHOLE_SIZE;

      buffer.state = state;
    }
  }

  // --------------------------
  // Create end dependency info
  // --------------------------
  VkDependencyInfo end_dep_info{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                .pNext = nullptr,
                                .dependencyFlags = 0u,
                                .memoryBarrierCount = 0,
                                .pMemoryBarriers = nullptr,
                                .bufferMemoryBarrierCount = static_cast<uint32_t>(end_buffer_barriers.size()),
                                .pBufferMemoryBarriers = end_buffer_barriers.data(),
                                .imageMemoryBarrierCount = static_cast<uint32_t>(end_image_barriers.size()),
                                .pImageMemoryBarriers = end_image_barriers.data()};

  // ------------------
  // Execute end states
  // ------------------
  vkCmdPipelineBarrier2(cmd, &end_dep_info);

  // -------------------
  // Reset supplied data
  // -------------------
  passes_.clear();
  resource_deps_.clear();
  end_image_states_.clear();
  end_buffer_states_.clear();
}

VkImageAspectFlags fwrk::Graph::get_aspect_for_format(const VkFormat format)
{
  switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
      return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_S8_UINT:
      return VK_IMAGE_ASPECT_STENCIL_BIT;
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default:
      return VK_IMAGE_ASPECT_COLOR_BIT;
  }
}

#include "pass_builder.hpp"

#include <cassert>

#include "graph.hpp"
#include "types/pass.hpp"

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_color_attachment(const AttachmentInfo& info)
{
  if (!try_access(info.resource.id)) return *this;
  auto& res = graph_->resource_deps_[info.resource.id];

  VkImageAspectFlags aspect = info.aspect;
  if (info.aspect == VK_IMAGE_ASPECT_NONE) {
    aspect |= VK_IMAGE_ASPECT_COLOR_BIT;
  }

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id,
      Attachment{.load_op = static_cast<VkAttachmentLoadOp>(info.load_op),
                 .store_op = static_cast<VkAttachmentStoreOp>(info.store_op),
                 .clear_value = info.clear_value},
      aspect, info.base_level, 1, info.base_layer, 1, info.view_type, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  if (info.load_op == LoadOp::Load && !res.write_passes.empty()) {
    res.read_passes.push_back(id_);
    set_possible_explicit_read(info.resource.pass, res);

    image.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
  }

  res.write_passes.push_back(id_);

  if (info.resolve) {
    if (!try_access(info.resolve->resource)) return *this;
    auto& res_olve = graph_->resource_deps_[info.resolve->resource];

    const auto idx = static_cast<uint32_t>(pass_->images.size());
    pass_->images.emplace_back(info.resolve->resource, std::nullopt, aspect, info.resolve->base_level, 1,
                               info.resolve->base_layer, 1, info.view_type, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                               VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    res_olve.write_passes.push_back(id_);

    image.attachment->resolve = idx;
    image.attachment->resolve_mode = info.resolve->mode;
  }

  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_depth_attachment(const AttachmentInfo& info)
{
  if (!try_access(info.resource.id)) return *this;
  auto& res = graph_->resource_deps_[info.resource.id];

  VkImageAspectFlags aspect = info.aspect;
  if (info.aspect == VK_IMAGE_ASPECT_NONE) {
    aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id,
      Attachment{.load_op = static_cast<VkAttachmentLoadOp>(info.load_op),
                 .store_op = static_cast<VkAttachmentStoreOp>(info.store_op),
                 .clear_value = info.clear_value},
      aspect, info.base_level, 1, info.base_layer, 1, info.view_type, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  if (info.load_op == LoadOp::Load && !res.write_passes.empty()) {
    res.read_passes.push_back(id_);
    set_possible_explicit_read(info.resource.pass, res);

    image.access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }

  res.write_passes.push_back(id_);

  if (info.resolve) {
    if (!try_access(info.resolve->resource)) return *this;
    auto& res_olve = graph_->resource_deps_[info.resolve->resource];

    const auto idx = static_cast<uint32_t>(pass_->images.size());
    pass_->images.emplace_back(
        info.resolve->resource, std::nullopt, aspect, info.resolve->base_level, 1, info.resolve->base_layer, 1,
        info.view_type, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    res_olve.write_passes.push_back(id_);

    image.attachment->resolve = idx;
    image.attachment->resolve_mode = info.resolve->mode;
  }
  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_vertex_buffer_input(const BufferInfo& info)
{
  set_buffer_read(info, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_index_buffer_input(const BufferInfo& info)
{
  set_buffer_read(info, VK_ACCESS_2_INDEX_READ_BIT);
  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_indirect_buffer_input(const BufferInfo& info)
{
  set_buffer_read(info, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_render_area(const VkExtent2D area)
{
  pass_->render_info.render_area = area;
  return *this;
}

fwrk::GraphicsPassBuilder& fwrk::GraphicsPassBuilder::set_render_view(const uint32_t layer_count,
                                                                      const uint32_t view_mask)
{
  pass_->render_info.layer_count = layer_count;
  pass_->render_info.view_mask = view_mask;
  return *this;
}

template<typename T>
T& fwrk::PassBuilder<T>::set_image_read(const ImageInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];
  res.read_passes.push_back(id_);
  set_possible_explicit_read(info.resource.pass, res);

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id, std::nullopt, info.aspect, info.base_level, info.level_count, info.base_layer, info.layer_count,
      info.view_type, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, info.stages, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  set_stages_fallback(image.stages);

  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_uniform_buffer_read(const BufferInfo& info)
{
  set_buffer_read(info, VK_ACCESS_2_UNIFORM_READ_BIT);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_storage_buffer_read(const BufferInfo& info)
{
  set_buffer_read(info, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_storage_buffer_write(const BufferInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  BufferAccess& buffer = pass_->buffers.emplace_back(info.resource.id, info.size, info.offset,
                                                     VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, info.stages);

  set_stages_fallback(buffer.stages);

  set_possible_read(info.resource, res, buffer.access, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

  res.write_passes.push_back(id_);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_storage_image_read(const StorageImageInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];
  res.read_passes.push_back(id_);
  set_possible_explicit_read(info.resource.pass, res);

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id, std::nullopt, info.aspect, info.base_level, 1, info.base_layer, info.layer_count,
      info.view_type, VK_ACCESS_2_SHADER_STORAGE_READ_BIT, info.stages, VK_IMAGE_LAYOUT_GENERAL);

  set_stages_fallback(image.stages);

  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_storage_image_write(const StorageImageInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id, std::nullopt, info.aspect, info.base_level, 1, info.base_layer, info.layer_count,
      info.view_type, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, info.stages, VK_IMAGE_LAYOUT_GENERAL);

  set_stages_fallback(image.stages);

  set_possible_read(info.resource, res, image.access, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

  res.write_passes.push_back(id_);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_image_transfer_src(const ImageInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  res.read_passes.push_back(id_);
  set_possible_explicit_read(info.resource.pass, res);

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id, std::nullopt, info.aspect, info.base_level, info.level_count, info.base_layer, info.layer_count,
      info.view_type, VK_ACCESS_2_TRANSFER_READ_BIT, info.stages, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  if (image.stages == VK_PIPELINE_STAGE_2_NONE) {
    image.stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  }

  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_image_transfer_dst(const ImageInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  ImageAccess& image = pass_->images.emplace_back(
      info.resource.id, std::nullopt, info.aspect, info.base_level, info.level_count, info.base_layer, info.layer_count,
      info.view_type, VK_ACCESS_2_TRANSFER_WRITE_BIT, info.stages, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  if (image.stages == VK_PIPELINE_STAGE_2_NONE) {
    image.stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  }

  res.write_passes.push_back(id_);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_buffer_transfer_src(const BufferInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  res.read_passes.push_back(id_);
  set_possible_explicit_read(info.resource.pass, res);

  BufferAccess& buffer =
      pass_->buffers.emplace_back(info.resource.id, info.size, info.offset, VK_ACCESS_2_TRANSFER_READ_BIT, info.stages);

  if (buffer.stages == VK_PIPELINE_STAGE_2_NONE) {
    buffer.stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  }

  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_buffer_transfer_dst(const BufferInfo& info)
{
  if (!try_access(info.resource.id)) return static_cast<T&>(*this);
  auto& res = graph_->resource_deps_[info.resource.id];

  BufferAccess& buffer = pass_->buffers.emplace_back(info.resource.id, info.size, info.offset,
                                                     VK_ACCESS_2_TRANSFER_WRITE_BIT, info.stages);

  if (buffer.stages == VK_PIPELINE_STAGE_2_NONE) {
    buffer.stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  }

  res.write_passes.push_back(id_);
  return static_cast<T&>(*this);
}

template<typename T>
T& fwrk::PassBuilder<T>::set_execute(std::function<void(VkCommandBuffer)> func)
{
  pass_->func = std::move(func);
  return static_cast<T&>(*this);
}

template<typename T>
void fwrk::PassBuilder<T>::set_buffer_read(const BufferInfo& info, const VkAccessFlags2 access)
{
  if (!try_access(info.resource.id)) return;
  auto& res = graph_->resource_deps_[info.resource.id];
  res.read_passes.push_back(id_);
  set_possible_explicit_read(info.resource.pass, res);

  BufferAccess& buffer = pass_->buffers.emplace_back(info.resource.id, info.size, info.offset, access, info.stages);

  set_stages_fallback(buffer.stages);
}

template<typename T>
void fwrk::PassBuilder<T>::set_stages_fallback(VkPipelineStageFlags2& stages) const
{
  if (stages == VK_PIPELINE_STAGE_2_NONE) {
    if constexpr (std::is_same_v<T, GraphicsPassBuilder>) {
      stages = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    } else if constexpr (std::is_same_v<T, ComputePassBuilder>) {
      stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    }
  }
}

template<typename T>
void fwrk::PassBuilder<T>::set_possible_read(const ResourceAccess& resource, ResourceDependencies& deps,
                                             VkAccessFlags2& access, const VkAccessFlags2 access_flags) const
{
  if (!deps.write_passes.empty()) {
    deps.read_passes.push_back(id_);
    set_possible_explicit_read(resource.pass, deps);
    access |= access_flags;
  }
}

template<typename T>
bool fwrk::PassBuilder<T>::try_access(const ResourceID resource)
{
  if (accessed_.contains(resource)) return false;
  accessed_.insert(resource);
  return true;
}

template<typename T>
void fwrk::PassBuilder<T>::set_possible_explicit_read(const std::optional<uint32_t> pass,
                                                      ResourceDependencies& deps) const
{
  if (pass) {
    assert(*pass < id_ && "Invalid pass id for explicit read dep");
    deps.read_deps[id_] = *pass;
  }
}

template class fwrk::PassBuilder<fwrk::GraphicsPassBuilder>;
template class fwrk::PassBuilder<fwrk::ComputePassBuilder>;

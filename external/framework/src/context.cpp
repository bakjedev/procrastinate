#include "context.hpp"

fwrk::Context::~Context()
{
  if (!device_) return;
  for (auto& views: views_cache_) {
    for (auto [_, view]: views) {
      if (view) {
        vkDestroyImageView(device_, view, nullptr);
      }
    }
  }
}

fwrk::ResourceID fwrk::Context::import_image(const ImageResource& image, VkImage raw, std::string name)
{
  if (raw == VK_NULL_HANDLE) {
    // return ResourceID{};
  }

  const auto slot_id = images_.size();
  images_.push_back(image);
  views_cache_.emplace_back();

  const auto raw_id = raw_images_.size();
  raw_images_.push_back(raw);

  const auto id = resources_.size();
  resources_.emplace_back(ResourceType::Image, slot_id, raw_id, std::move(name));

  return ResourceID{id};
}

fwrk::ResourceID fwrk::Context::import_buffer(const BufferResource& buffer, VkBuffer raw, std::string name)
{
  if (raw == VK_NULL_HANDLE) {
    // return ResourceID{};
  }

  const auto slot_id = buffers_.size();
  buffers_.push_back(buffer);

  const auto raw_id = raw_buffers_.size();
  raw_buffers_.push_back(raw);

  const auto id = resources_.size();
  resources_.emplace_back(ResourceType::Buffer, slot_id, raw_id, std::move(name));

  return ResourceID{id};
}

void fwrk::Context::update_image(const ResourceID resource, const ImageResource& image, VkImage raw)
{
  if (!resource) return;
  const Resource& res = resources_[*resource.id];
  ImageResource& img = images_[res.slot];

  destroy_views(res.slot);
  img = image;
  raw_images_[res.raw] = raw;
}

void fwrk::Context::update_buffer(const ResourceID resource, const BufferResource& buffer, VkBuffer raw)
{
  if (!resource) return;
  const Resource& res = resources_[*resource.id];
  BufferResource& buf = buffers_[res.slot];

  buf = buffer;
  raw_buffers_[res.raw] = raw;
}

fwrk::ResourceID fwrk::Context::create_proxy(const ResourceID resource, std::string name)
{
  if (resource && resources_[*resource.id].type == ResourceType::Proxy) return {};

  const auto id = resources_.size();
  resources_.emplace_back(ResourceType::Proxy, resource.id.value_or(UINT32_MAX), std::move(name));

  return ResourceID{id};
}

void fwrk::Context::update_proxy(const ResourceID proxy, const ResourceID resource)
{
  if (!proxy.id || !resource.id) return;
  if (resources_[*resource.id].type == ResourceType::Proxy) return;
  resources_[*proxy.id].target = *resource.id;
}

VkImageView fwrk::Context::get_image_view(const ViewKey& key, const Resource& resource)
{
  if (device_ == VK_NULL_HANDLE) return VK_NULL_HANDLE;
  VkImage image_raw = raw_images_[resource.raw];
  const ImageResource& image = images_[resource.slot];

  auto& views = views_cache_[resource.slot];

  auto it = views.find(key);
  if (it != views.end()) {
    return it->second;
  }

  const VkImageViewCreateInfo view_create_info{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                               .pNext = nullptr,
                                               .flags = 0u,
                                               .image = image_raw,
                                               .viewType = key.view_type,
                                               .format = image.format,
                                               .components = {},
                                               .subresourceRange = {.aspectMask = key.aspect,
                                                                    .baseMipLevel = key.base_level,
                                                                    .levelCount = key.level_count,
                                                                    .baseArrayLayer = key.base_layer,
                                                                    .layerCount = key.layer_count}};

  VkImageView view = VK_NULL_HANDLE;
  if (vkCreateImageView(device_, &view_create_info, nullptr, &view) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  views[key] = view;

  return view;
}

void fwrk::Context::destroy_views(const uint32_t slot)
{
  if (device_ == VK_NULL_HANDLE) return;

  auto& views = views_cache_[slot];

  for (auto [_, view]: views) {
    if (view) {
      vkDestroyImageView(device_, view, nullptr);
    }
  }
  views.clear();
}

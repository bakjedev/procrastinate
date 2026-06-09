#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "graph.hpp"
#include "types/resource.hpp"
#include "util/flat_hash_map.hpp"

namespace fwrk {
  template<class T>
  static void hash_combine(size_t& seed, const T& value)
  {
    std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }

  struct ViewKey {
    VkImageAspectFlags aspect;
    uint32_t base_level;
    uint32_t level_count;
    uint32_t base_layer;
    uint32_t layer_count;
    VkImageViewType view_type;

    ViewKey(const VkImageSubresourceRange& sub, const VkImageViewType view_type_) :
        aspect(sub.aspectMask), base_level(sub.baseMipLevel), level_count(sub.levelCount),
        base_layer(sub.baseArrayLayer), layer_count(sub.layerCount), view_type(view_type_)
    {
    }

    bool operator==(const ViewKey&) const = default;
  };

  struct ViewKeyHasher {
    size_t operator()(const ViewKey& key) const
    {
      size_t seed = 0;
      hash_combine(seed, key.aspect);
      hash_combine(seed, key.base_level);
      hash_combine(seed, key.level_count);
      hash_combine(seed, key.base_layer);
      hash_combine(seed, key.layer_count);
      hash_combine(seed, key.view_type);
      return seed;
    }
  };

  class Context {
  public:
    explicit Context(VkDevice device) : device_(device) {}
    ~Context();

    [[nodiscard]] ResourceID import_image(const ImageResource& image, VkImage raw, std::string name = "Unnamed image");

    [[nodiscard]] ResourceID import_buffer(const BufferResource& buffer, VkBuffer raw,
                                           std::string name = "Unnamed buffer");

    template<ImageInterface I>
    [[nodiscard]] ResourceID import_image(const I& image, const ImageState& state, std::string name = "Unnamed image");

    template<BufferInterface I>
    [[nodiscard]] ResourceID import_buffer(const I& buffer, const BufferState& state,
                                           std::string name = "Unnamed buffer");

    void update_image(ResourceID resource, const ImageResource& image, VkImage raw);
    void update_buffer(ResourceID resource, const BufferResource& buffer, VkBuffer raw);

    template<ImageInterface I>
    void update_image(ResourceID resource, const I& image, const ImageState& state);

    template<BufferInterface I>
    void update_buffer(ResourceID resource, const I& buffer, const BufferState& state);

    [[nodiscard]] ResourceID create_proxy(ResourceID resource = {}, std::string name = "Unnamed proxy");
    void update_proxy(ResourceID proxy, ResourceID resource);

    [[nodiscard]] Graph& graph() { return graph_; }


  private:
    friend Graph;
    std::vector<Resource> resources_;

    std::vector<ImageResource> images_;
    std::vector<BufferResource> buffers_;

    std::vector<VkImage> raw_images_;
    std::vector<VkBuffer> raw_buffers_;

    VkDevice device_ = VK_NULL_HANDLE;

    Graph graph_{this};

    std::vector<flat_hash_map<ViewKey, VkImageView, ViewKeyHasher>> views_cache_{};

    VkImageView get_image_view(const ViewKey& key, const Resource& resource);
    void destroy_views(uint32_t slot);

    template<ImageInterface I>
    static ImageResource construct_image(const I& image, const ImageState& state);
  };
} // namespace fwrk

#include "context.tpp"

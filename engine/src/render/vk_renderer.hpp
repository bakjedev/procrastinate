#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "util/frustum.hpp"


struct MeshResource;
struct DebugLineVertex;
struct RenderObject;
class VulkanFrame;
class VulkanBuffer;
class VulkanImage;
class VulkanPipeline;
class VulkanPipelineLayout;
class VulkanDescriptorSetLayout;
class VulkanDescriptorPool;
class VulkanShader;
class VulkanCommandPool;
class VulkanSwapChain;
class VulkanAllocator;
class Window;
class ResourceManager;
class EventManager;
class VulkanInstance;
class VulkanSurface;
class VulkanDevice;

struct MeshInfo
{
  glm::vec3 b_min;
  float padding1{};
  glm::vec3 b_max;
  uint32_t index_count;
  uint32_t first_index;
  int32_t vertex_offset;
  std::array<int32_t, 2> padding2{};
};

struct TextureInfo
{
  uint32_t texture_id;
  int32_t width;
  int32_t height;
};

struct PushConstant
{
  glm::mat4 view;
  glm::mat4 proj;
};

struct ComputePushConstant
{
  Frustum frustum;
  uint32_t render_object_count;
};

struct Vertex
{
  glm::vec3 position;
  float padding1{};
  glm::vec3 color;
  float padding2{};
  glm::vec3 normal;
  float padding3{};
  glm::vec2 tex_coord;
  std::array<float, 2> padding4{};
};

struct RenderObject
{
  glm::mat4 model;
  uint32_t mesh_id;
  int32_t texture_id;
  std::array<int32_t, 2> pad;
};

struct DebugLineVertex
{
  glm::vec3 position;
  float pad1{};
  glm::vec3 color;
  float pad2{};
};

class VulkanRenderer
{
public:
  explicit VulkanRenderer(Window *window, ResourceManager &resource_manager, EventManager &event_manager);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = delete;
  ~VulkanRenderer();

  void run(glm::mat4 world, float fov);

  uint32_t AddMesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices, uint32_t first_index,
                   int32_t vertex_offset, const glm::vec3 &b_min, const glm::vec3 &b_max);

  uint32_t AddTexture(std::span<const unsigned char> texture, int32_t width, int32_t height);

  void Upload();

  void RenderMesh(const glm::mat4 &model, uint32_t mesh_id, int32_t texture_id = -1);
  void ClearMeshes(uint32_t reserve = 0);

  void RenderLine(const glm::vec3 &point_a, const glm::vec3 &point_b, const glm::vec3 &color);
  void ClearLines();
  [[nodiscard]] int32_t GetVertexCount() const;
  [[nodiscard]] uint32_t GetIndexCount() const;

  void OnMeshResourceDestroyed(const MeshResource &resource);

private:
  [[nodiscard]] std::optional<uint32_t> BeginFrame() const;
  void EndFrame(uint32_t image_index);

  void RecreateSwapChain();
  void RecreateFrameImages(uint32_t width, uint32_t height) const;

  static constexpr uint32_t max_frames_in_flight_ = 2;

  std::unique_ptr<VulkanInstance> instance_;
  std::unique_ptr<VulkanSurface> surface_;
  std::unique_ptr<VulkanDevice> device_;
  std::unique_ptr<VulkanAllocator> allocator_;

  std::unique_ptr<VulkanSwapChain> swap_chain_;

  std::unique_ptr<VulkanCommandPool> graphics_pool_;
  std::unique_ptr<VulkanCommandPool> transfer_pool_;

  std::unique_ptr<VulkanShader> pre_pass_vert_;
  std::unique_ptr<VulkanShader> pre_pass_frag_;
  std::unique_ptr<VulkanShader> culling_comp_;
  std::unique_ptr<VulkanShader> shading_comp_;

  std::unique_ptr<VulkanShader> debug_line_vert_;
  std::unique_ptr<VulkanShader> debug_line_frag_;

  std::unique_ptr<VulkanDescriptorPool> descriptor_pool_;

  std::unique_ptr<VulkanDescriptorSetLayout> static_descriptor_set_layout_;
  std::unique_ptr<VulkanDescriptorSetLayout> frame_descriptor_set_layout_;
  vk::DescriptorSet static_descriptor_set_;

  std::unique_ptr<VulkanPipelineLayout> pre_pass_pipeline_layout_;
  std::unique_ptr<VulkanPipeline> pre_pass_pipeline_;
  std::unique_ptr<VulkanPipelineLayout> debug_line_pipeline_layout_;
  std::unique_ptr<VulkanPipeline> debug_line_pipeline_;
  std::unique_ptr<VulkanPipelineLayout> culling_pipeline_layout_;
  std::unique_ptr<VulkanPipeline> culling_pipeline_;
  std::unique_ptr<VulkanPipelineLayout> shading_pipeline_layout_;
  std::unique_ptr<VulkanPipeline> shading_pipeline_;

  // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  std::vector<std::unique_ptr<VulkanFrame>> frames_;
  std::vector<vk::UniqueSemaphore> submit_semaphores_;

  uint32_t current_frame_ = 0;
  float aspect_ratio_ = 1.0F;

  std::vector<RenderObject> render_objects_;

  std::vector<DebugLineVertex> debug_line_vertices_;

  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
  std::vector<MeshInfo> mesh_infos_;
  std::unique_ptr<VulkanBuffer> vertex_buffer_;
  std::unique_ptr<VulkanBuffer> index_buffer_;
  std::unique_ptr<VulkanBuffer> mesh_info_buffer_;
  vk::UniqueSampler visibility_sampler_;

  std::vector<std::vector<unsigned char>> textures_;
  std::vector<TextureInfo> texture_infos_;
  std::vector<std::unique_ptr<VulkanImage>> texture_images_;
  vk::UniqueSampler texture_sampler_;


  Window *window_ = nullptr;
  EventManager *event_manager_ = nullptr;
};

#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "util/frustum.hpp"

struct DebugLineVertex;
struct RenderObject;
class VulkanFrame;
class VulkanBuffer;
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

struct MeshInfo {
  glm::vec3 bmin;
  uint32_t indexCount;
  glm::vec3 bmax;
  uint32_t firstIndex;
  int32_t vertexOffset;
  std::array<float, 3> pad;
};

struct PushConstant {
  glm::mat4 view;
  glm::mat4 proj;
};

struct ComputePushConstant {
  Frustum frustum;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
  glm::vec3 normal;
};

class VulkanRenderer {
 public:
  explicit VulkanRenderer(Window *window, ResourceManager &resourceManager,
                          EventManager &eventManager);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = delete;
  ~VulkanRenderer();

  void run(glm::mat4 world, float fov);

  uint32_t addMesh(const std::vector<Vertex> &vertices,
                   const std::vector<uint32_t> &indices, uint32_t firstIndex,
                   int32_t vertexOffset, const glm::vec3 &bmin,
                   const glm::vec3 &bmax);

  void upload();

  void renderMesh(glm::mat4 model, uint32_t meshID);
  void clearMeshes();

  void renderLine(const glm::vec3 &pointA, const glm::vec3 &pointB,
                  const glm::vec3 &color);
  void clearLines();
  [[nodiscard]] int32_t getVertexCount() const;
  [[nodiscard]] uint32_t getIndexCount() const;

 private:
  [[nodiscard]] std::optional<uint32_t> beginFrame() const;
  void endFrame(uint32_t imageIndex);

  void recreateSwapchain();
  void recreateDepthImage(uint32_t width, uint32_t height) const;

  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  std::unique_ptr<VulkanInstance> m_instance;
  std::unique_ptr<VulkanSurface> m_surface;
  std::unique_ptr<VulkanDevice> m_device;
  std::unique_ptr<VulkanAllocator> m_allocator;

  std::unique_ptr<VulkanSwapChain> m_swapChain;

  std::unique_ptr<VulkanCommandPool> m_graphicsPool;
  std::unique_ptr<VulkanCommandPool> m_transferPool;
  std::unique_ptr<VulkanCommandPool> m_computePool;

  std::unique_ptr<VulkanShader> m_vertexShader;
  std::unique_ptr<VulkanShader> m_fragmentShader;
  std::unique_ptr<VulkanShader> m_computeShader;

  std::unique_ptr<VulkanShader> m_debugLineVert;
  std::unique_ptr<VulkanShader> m_debugLineFrag;

  std::unique_ptr<VulkanDescriptorPool> m_descriptorPool;

  std::unique_ptr<VulkanDescriptorSetLayout> m_staticDescriptorSetLayout;
  std::unique_ptr<VulkanDescriptorSetLayout> m_frameDescriptorSetLayout;
  vk::DescriptorSet m_staticDescriptorSet;

  std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
  std::unique_ptr<VulkanPipeline> m_pipeline;
  std::unique_ptr<VulkanPipelineLayout> m_debugLinePipelineLayout;
  std::unique_ptr<VulkanPipeline> m_debugLinePipeline;
  std::unique_ptr<VulkanPipelineLayout> m_compPipelineLayout;
  std::unique_ptr<VulkanPipeline> m_compPipeline;

  std::vector<std::unique_ptr<VulkanFrame>> m_frames;

  uint32_t m_currentFrame = 0;
  float m_aspectRatio = 1.0F;

  std::vector<RenderObject> m_renderObjects;

  std::vector<DebugLineVertex> m_debugLineVertices;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
  std::vector<MeshInfo> m_meshInfos;
  std::unique_ptr<VulkanBuffer> m_vertexBuffer;
  std::unique_ptr<VulkanBuffer> m_indexBuffer;
  std::unique_ptr<VulkanBuffer> m_meshInfoBuffer;

  Window *m_window = nullptr;
  EventManager *m_eventManager = nullptr;
};

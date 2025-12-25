#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>

#include "render/vk_allocator.hpp"
#include "render/vk_command_pool.hpp"
#include "render/vk_descriptor.hpp"
#include "render/vk_device.hpp"
#include "render/vk_frame.hpp"
#include "render/vk_instance.hpp"
#include "render/vk_pipeline.hpp"
#include "render/vk_shader.hpp"
#include "render/vk_surface.hpp"
#include "render/vk_swap_chain.hpp"
#include "vk_buffer.hpp"

class Window;

class ResourceManager;
class EventManager;

struct MeshInfo {
  uint32_t indexCount;
  uint32_t firstIndex;
  int32_t vertexOffset;
};

struct PushConstant {
  glm::mat4 view;
  glm::mat4 proj;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
};

class VulkanRenderer {
 public:
  explicit VulkanRenderer(Window *window, ResourceManager &resourceManager, EventManager& eventManager);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = delete;
  ~VulkanRenderer();

  void run();

  uint32_t addMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t firstIndex, int32_t vertexOffset);
  void upload();

  void renderMesh(glm::mat4 model, uint32_t meshID);
  void clearMeshes();

  [[nodiscard]] int32_t getVertexCount() const;
  [[nodiscard]] uint32_t getIndexCount() const;

 private:
  std::optional<uint32_t> beginFrame();
  void endFrame(uint32_t imageIndex);

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

  std::unique_ptr<VulkanDescriptorPool> m_descriptorPool;

  std::unique_ptr<VulkanDescriptorSetLayout> m_staticDescriptorSetLayout;
  std::unique_ptr<VulkanDescriptorSetLayout> m_frameDescriptorSetLayout;
  vk::DescriptorSet m_staticDescriptorSet;

  std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
  std::unique_ptr<VulkanPipeline> m_pipeline;
  std::unique_ptr<VulkanPipelineLayout> m_compPipelineLayout;
  std::unique_ptr<VulkanPipeline> m_compPipeline;

  std::vector<std::unique_ptr<VulkanFrame>> m_frames;
  std::vector<vk::UniqueSemaphore> m_imageAvailableSemaphores;
  std::vector<vk::UniqueFence> m_inFlightFences;
  uint32_t m_currentFrame = 0;

  std::vector<RenderObject> m_renderObjects;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
  std::vector<MeshInfo> m_meshInfos;
  std::unique_ptr<VulkanBuffer> m_vertexBuffer;
  std::unique_ptr<VulkanBuffer> m_indexBuffer;
  std::unique_ptr<VulkanBuffer> m_meshInfoBuffer;


  Window* m_window = nullptr;
  EventManager* m_eventManager = nullptr;
};

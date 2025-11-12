#pragma once
#include <memory>

#include "render/vk_allocator.hpp"
#include "render/vk_command_pool.hpp"
#include "render/vk_device.hpp"
#include "render/vk_frame.hpp"
#include "render/vk_instance.hpp"
#include "render/vk_pipeline.hpp"
#include "render/vk_shader.hpp"
#include "render/vk_surface.hpp"
#include "render/vk_swap_chain.hpp"
#include "vk_buffer.hpp"

struct SDL_Window;

class ResourceManager;

struct Vertex {
  float x;
  float y;
  float z;
};

class VulkanRenderer {
 public:
  explicit VulkanRenderer(SDL_Window *window, ResourceManager &resourceManager);
  VulkanRenderer(const VulkanRenderer &) = delete;
  VulkanRenderer(VulkanRenderer &&) = delete;
  VulkanRenderer &operator=(const VulkanRenderer &) = delete;
  VulkanRenderer &operator=(VulkanRenderer &&) = delete;
  ~VulkanRenderer();

  void run();

  void addVertices(const std::vector<Vertex> &vertices);
  void addIndices(const std::vector<uint32_t> &indices);
  void upload();

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

  std::unique_ptr<VulkanPipelineLayout> m_pipelineLayout;
  std::unique_ptr<VulkanPipeline> m_pipeline;

  std::vector<std::unique_ptr<VulkanFrame>> m_frames;
  std::vector<vk::UniqueSemaphore> m_imageAvailableSemaphores;
  std::vector<vk::UniqueFence> m_inFlightFences;
  uint32_t m_currentFrame = 0;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
  std::unique_ptr<VulkanBuffer> m_vertexBuffer;
  std::unique_ptr<VulkanBuffer> m_indexBuffer;
};
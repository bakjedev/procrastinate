#pragma once
#include <vulkan/vulkan.h>

#include <vector>

struct DescriptorPoolInfo {
  uint32_t maxSets{};
  std::vector<VkDescriptorPoolSize> poolSizes;
  VkDescriptorPoolCreateFlags flags{};
};

class VulkanDescriptorPool {
 public:
  VulkanDescriptorPool(VkDevice device, const DescriptorPoolInfo &info);
  VulkanDescriptorPool(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool(VulkanDescriptorPool &&) = delete;
  VulkanDescriptorPool &operator=(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool &operator=(VulkanDescriptorPool &&) = delete;
  ~VulkanDescriptorPool();

  VkDescriptorSet allocate(VkDescriptorSetLayout layout);
  std::vector<VkDescriptorSet> allocate(
      const std::vector<VkDescriptorSetLayout> &layouts);

 private:
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;
};

class VulkanDescriptorSetLayout {
 public:
  VulkanDescriptorSetLayout(
      VkDevice device,
      const std::vector<VkDescriptorSetLayoutBinding> &bindings);
  VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout &) = delete;
  VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&) = delete;
  VulkanDescriptorSetLayout &operator=(const VulkanDescriptorSetLayout &) =
      delete;
  VulkanDescriptorSetLayout &operator=(VulkanDescriptorSetLayout &&) = delete;
  ~VulkanDescriptorSetLayout();

 private:
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;

  VkDevice m_device = VK_NULL_HANDLE;
};
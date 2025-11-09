#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

struct DescriptorPoolInfo {
  vk::DescriptorPoolCreateFlags flags;
  uint32_t maxSets;
  std::vector<vk::DescriptorPoolSize> poolSizes;
};

class VulkanDescriptorPool {
 public:
  VulkanDescriptorPool(vk::Device device, const DescriptorPoolInfo &info);
  VulkanDescriptorPool(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool(VulkanDescriptorPool &&) = delete;
  VulkanDescriptorPool &operator=(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool &operator=(VulkanDescriptorPool &&) = delete;
  ~VulkanDescriptorPool();

  vk::DescriptorSet allocate(vk::DescriptorSetLayout layout);
  std::vector<vk::DescriptorSet> allocate(
      const std::vector<vk::DescriptorSetLayout> &layouts);

 private:
  vk::Device m_device;
  vk::DescriptorPool m_descriptorPool;
};

class VulkanDescriptorSetLayout {
 public:
  VulkanDescriptorSetLayout(
      vk::Device device,
      const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
      const std::vector<vk::DescriptorBindingFlags> &bindingFlags = {},
      vk::DescriptorSetLayoutCreateFlags flags = {});
  VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout &) = delete;
  VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&) = delete;
  VulkanDescriptorSetLayout &operator=(const VulkanDescriptorSetLayout &) =
      delete;
  VulkanDescriptorSetLayout &operator=(VulkanDescriptorSetLayout &&) = delete;
  ~VulkanDescriptorSetLayout();

  [[nodiscard]] vk::DescriptorSetLayout get() const { return m_layout; }

 private:
  vk::Device m_device;
  vk::DescriptorSetLayout m_layout;
};
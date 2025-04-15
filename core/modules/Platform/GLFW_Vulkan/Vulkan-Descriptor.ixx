module;

#include <vector>
#include <vulkan/vulkan.h>

export module Vulkan:Descriptor;

import :Device;

export
{
	struct VulkanDescriptorSetLayout
	{
		static VulkanDescriptorSetLayout Create(
			const VulkanDevice* logical_device,
			const std::vector<VkDescriptorSetLayoutBinding>& bindings,
			const VkDescriptorSetLayoutCreateFlags& create_flags = 0,
			void* extension = nullptr
		);

		static void Destroy(const VulkanDevice* logical_device, VulkanDescriptorSetLayout& layout);

		VkDescriptorSetLayout handle = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	struct VulkanDescriptorPool
	{
		static VulkanDescriptorPool Create(
			const VulkanDevice* logical_device,
			const uint32_t& max_sets, 
			const std::vector<VkDescriptorPoolSize>& pool_sizes, 
			const VkDescriptorPoolCreateFlags& create_flags = 0, 
			void* extension = nullptr
		);

		static std::vector<VkDescriptorSet> Allocate(
			const VulkanDevice* logical_device, 
			const VulkanDescriptorPool& pool, 
			const VulkanDescriptorSetLayout& layout, 
			const uint32_t& count
		);

		static void Destroy(const VulkanDevice* logical_device, VulkanDescriptorPool& pool);

		VkDescriptorPool handle = VK_NULL_HANDLE;
	};
}
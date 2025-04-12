module;

#include <optional>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

export module Vulkan:Device;

export
{
	struct VulkanDeviceConfiguration
	{
		VkPhysicalDeviceFeatures2 features{};
		VkPhysicalDeviceType device_type;
		VmaAllocatorCreateFlags allocator_flags = 0;
		std::vector<const char*> extensions;
		std::vector<const char*> layers;
	};

	struct VulkanDeviceRequirements
	{
		VkPhysicalDeviceFeatures2 features{};
		VkPhysicalDeviceType device_type;
		VmaAllocatorCreateFlags allocator_flags = 0;
		std::vector<const char*> extensions;
		std::vector<const char*> layers;
	};

	struct VulkanDevice
	{
		static VulkanDevice Create(const VkInstance& instance, const VulkanDeviceConfiguration& reqs);
		static bool MeetsRequirements(const VkPhysicalDevice& physical_device, const VulkanDeviceConfiguration& reqs);

		VkInstance vk_instance = VK_NULL_HANDLE;
		VkDevice handle = VK_NULL_HANDLE;
		VmaAllocator allocator = VK_NULL_HANDLE;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties2 properties{};
		VkPhysicalDeviceFeatures2 features{};

		VkCommandPool immediate_cmd_pool;
		VkCommandBuffer immediate_cmd_buffer;
		VkFence immediate_fence;
		VkSemaphore immediate_semaphore;

		VkQueue graphics_queue = VK_NULL_HANDLE;
		std::optional<uint32_t> graphics_queue_index;
		VkQueue compute_queue = VK_NULL_HANDLE;
		std::optional<uint32_t> compute_queue_index;

	};
}
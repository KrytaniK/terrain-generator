module;

#include <vector>
#include <cstdint>

#include <vulkan/vulkan.h>

export module Vulkan:Swapchain;

export
{
	struct VulkanSwapchainSupport
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	struct VulkanSwapchain
	{
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		VulkanSwapchainSupport support{};
		VkExtent2D extent{};
		VkSurfaceFormatKHR format{};
		VkPresentModeKHR present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		std::vector<VkImage> images;
		uint32_t current_image_index = 0;
	};
}
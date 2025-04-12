module;

#include <vulkan/vulkan.h>

export module Vulkan:Frame;

import :Image;

export
{
	struct VulkanFrameConstants
	{
		VkCommandBufferBeginInfo cmd_begin_info{};
		VkCommandBufferSubmitInfo graphics_buffer_submit_info{};
		VkSubmitInfo2 grahpics_submit_info{};
	};

	struct VulkanFrame
	{
		uint32_t generation = 0;

		VulkanImage image{};

		VkCommandPool compute_cmd_pool = VK_NULL_HANDLE;
		VkCommandPool graphics_cmd_pool = VK_NULL_HANDLE;
		VkCommandBuffer compute_cmd_buffer = VK_NULL_HANDLE;
		VkCommandBuffer graphics_cmd_buffer = VK_NULL_HANDLE;

		VkFence compute_fence = VK_NULL_HANDLE;
		VkFence graphics_fence = VK_NULL_HANDLE;

		VkSemaphore compute_semaphore = VK_NULL_HANDLE;
		VkSemaphore graphics_semaphore = VK_NULL_HANDLE;
		VkSemaphore swapchain_semaphore = VK_NULL_HANDLE;
	};
}
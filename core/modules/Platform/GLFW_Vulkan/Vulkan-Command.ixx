module;

#include <vulkan/vulkan.h>

export module Vulkan:Command;

import Aurion.Window;

import :Image;
import :Frame;

export
{
	struct VulkanCommand
	{
		const Aurion::WindowHandle& window_handle;
		
		const VkCommandBuffer& graphics_buffer;
		const VkCommandBuffer& compute_buffer;

		const VkImage& render_image;
		const VkImageView& render_view;
		const VkSampler& render_sampler;
		const VkExtent3D& render_extent;
		const VkFormat& render_format;

		const VkExtent2D& swapchain_extent;

		const size_t& current_frame;
	};
}
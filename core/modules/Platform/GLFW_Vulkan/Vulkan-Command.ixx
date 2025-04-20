module;

#include <vulkan/vulkan.h>
#include <imgui.h>

export module Vulkan:Command;

import Graphics;

import :Image;
import :Frame;

export
{
	struct VulkanCommand : public IGraphicsCommand
	{
		const VkCommandBuffer& graphics_buffer;
		const VkCommandBuffer& compute_buffer;

		const VulkanImage& color_image;
		const VulkanImage& depth_image;
		const VulkanImage& resolve_image;

		const VkExtent2D& swapchain_extent;
	};
}
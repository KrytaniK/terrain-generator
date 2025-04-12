module;

#include <vulkan/vulkan.h>
#include <imgui.h>

export module Vulkan:Command;

import Graphics;

import :Image;
import :Frame;

export
{
	struct VulkanRenderCommand : public IGraphicsCommand
	{
		const VkCommandBuffer& graphics_buffer;
		const VkCommandBuffer& compute_buffer;

		const VkImageView& render_view;
		const VkSampler& render_sampler;

		const VkExtent3D& render_extent;
		const VkFormat& render_format;
	};

	struct VulkanUICommand : public IGraphicsCommand
	{
		const VkCommandBuffer& graphics_buffer;
		const VkCommandBuffer& compute_buffer;

		const VkImageView& render_view;
		const VkSampler& render_sampler;

		ImTextureID render_texture;
	};

	struct VulkanCommand : public IGraphicsCommand
	{
		const VkCommandBuffer& graphics_buffer;
		const VkCommandBuffer& compute_buffer;

		const VkImage& render_image;
		const VkImageView& render_view;
		const VkSampler& render_sampler;
		const VkExtent3D& render_extent;
		const VkFormat& render_format;

		const VkExtent2D& swapchain_extent;
	};
}
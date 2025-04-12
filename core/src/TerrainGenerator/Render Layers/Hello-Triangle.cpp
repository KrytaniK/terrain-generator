#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>

import HelloTriangle;
import Vulkan;

HelloTriangleLayer::HelloTriangleLayer()
	: m_enabled(true), m_pipeline(nullptr)
{

}

HelloTriangleLayer::~HelloTriangleLayer()
{

}

void HelloTriangleLayer::Initialize()
{

}

void HelloTriangleLayer::Record(const IGraphicsCommand* command)
{
	if (!m_enabled || !m_pipeline)
		return;

	VulkanRenderCommand* cmd = (VulkanRenderCommand*)(command);

	// Draw Triangle
	VkRenderingAttachmentInfo color_attachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = cmd->render_view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = VkClearValue{
			.color = VkClearColorValue{
				0.0f,
				0.0f,
				0.0f,
				1.0f
			}
		}
	};

	VkRenderingInfo render_info{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = VkRect2D{
			.extent = VkExtent2D{ cmd->render_extent.width, cmd->render_extent.height }
		},
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment
	};

	vkCmdBeginRendering(cmd->graphics_buffer, &render_info);

	vkCmdBindPipeline(cmd->graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->handle);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(cmd->render_extent.width);
	viewport.height = static_cast<float>(cmd->render_extent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd->graphics_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = cmd->render_extent.width;
	scissor.extent.height = cmd->render_extent.height;

	vkCmdSetScissor(cmd->graphics_buffer, 0, 1, &scissor);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd->graphics_buffer, 3, 1, 0, 0);

	vkCmdEndRendering(cmd->graphics_buffer);
}

void HelloTriangleLayer::Enable()
{
	m_enabled = true;
}

void HelloTriangleLayer::Disable()
{
	m_enabled = false;
}

void HelloTriangleLayer::SetGraphicsPipeline(const VulkanPipeline& pipeline)
{
	m_pipeline = &pipeline;
}

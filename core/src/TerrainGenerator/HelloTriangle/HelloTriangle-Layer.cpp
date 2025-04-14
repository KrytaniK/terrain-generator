#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>

import HelloTriangle;
import Vulkan;

HelloTriangleLayer::HelloTriangleLayer()
	: m_enabled(true), m_pipeline({})
{

}

HelloTriangleLayer::~HelloTriangleLayer()
{

}

void HelloTriangleLayer::Initialize(VulkanRenderer* renderer)
{
	m_logical_device = renderer->GetLogicalDevice();

	// Build Pipeline
	VulkanPipelineFactory pipeline_factory;
	pipeline_factory.Initialize(m_logical_device, renderer->GetVkPipelineBuffer());

	pipeline_factory.Configure<VulkanGraphicsPipeline>()
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/HelloTriangle/V-HelloTriangle.vert", false))
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/HelloTriangle/F-HelloTriangle.frag", false))
		.ConfigureVertexInputState()
		.ConfigureInputAssemblyState()
			.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.ConfigureRasterizationState()
			.SetPolygonMode(VK_POLYGON_MODE_FILL)
			.SetCullMode(VK_CULL_MODE_BACK_BIT)
			.SetFrontFace(VK_FRONT_FACE_CLOCKWISE)
			.SetLineWidth(1.0f)
		.ConfigureColorBlendState()
			.SetLogicOpEnabled(VK_FALSE)
			.SetBlendConstants(0.f, 0.f, 0.f, 0.f)
			.AddColorAttachment()
				.SetBlendEnabled(VK_FALSE)
				.SetSrcColorBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetDstColorBlendFactor(VK_BLEND_FACTOR_ZERO)
				.SetColorBlendOp(VK_BLEND_OP_ADD)
				.SetSrcAlphaBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetDstAlphaBlendFactor(VK_BLEND_FACTOR_ZERO)
				.SetAlphaBlendOp(VK_BLEND_OP_ADD)
				.SetColorWriteMask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
		.ConfigureViewportState()
			.AddViewport(VkViewport{})
			.AddScissor(VkRect2D{})
		.ConfigureMultisampleState()
			.SetSampleShadingEnabled(VK_FALSE)
			.SetRasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
			.SetMinSampleShading(1.0f)
			.SetAlphaToCoverageEnabled(VK_FALSE)
			.SetAlphaToOneEnabled(VK_FALSE)
		.ConfigureDynamicState()
			.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
			.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.AddDynamicColorAttachmentFormat(VK_FORMAT_B8G8R8A8_UNORM)
		.SetDynamicDepthAttachmentFormat(VK_FORMAT_UNDEFINED)
		.SetDynamicStencilAttachmentFormat(VK_FORMAT_UNDEFINED)
		.ConfigurePipelineLayout();

	m_pipeline = pipeline_factory.Build()[0];
}

void HelloTriangleLayer::Record(const IGraphicsCommand* command)
{
	if (!m_enabled || m_pipeline.handle == VK_NULL_HANDLE)
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

	vkCmdBindPipeline(cmd->graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);

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

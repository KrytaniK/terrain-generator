#include  <macros/AurionLog.h>

#include <vulkan/vulkan.h>

import HelloSquare;
import Vulkan;
import Resources;

HelloSquareLayer::HelloSquareLayer()
	: m_enabled(true), m_pipeline({})
{
	// Generate Vertex Data (Triangle Currently)
	m_vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
	};

	// Generate Index Data
	m_indices = { 0, 1, 2, 2, 3, 0 };
}

HelloSquareLayer::~HelloSquareLayer()
{
	VulkanBuffer::Destroy(m_logical_device, m_staging_buffer);
	VulkanBuffer::Destroy(m_logical_device, m_combined_buffer);
}

void HelloSquareLayer::Initialize(VulkanRenderer* renderer)
{
	m_logical_device = renderer->GetLogicalDevice();

	// Want to use a single buffer for vertex/index data
	{
		size_t buffer_size = (sizeof(Vertex) * m_vertices.size()) + (sizeof(uint32_t) * m_indices.size());

		// Generate CPU-side 'staging' buffer
		m_staging_buffer = VulkanBuffer::Create(m_logical_device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE);
		
		VkMemoryRequirements staging_reqs = VulkanBuffer::GetMemoryRequirements(m_logical_device, m_staging_buffer);
		VulkanBuffer::Allocate(
			m_logical_device,
			m_staging_buffer,
			staging_reqs,
			VulkanBuffer::FindMemoryType(
				m_logical_device, 
				m_staging_buffer, 
				staging_reqs.memoryTypeBits, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			)
		);

		// Generate GPU-side Buffer
		m_combined_buffer = VulkanBuffer::Create(
			m_logical_device,
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_SHARING_MODE_EXCLUSIVE
		);

		VkMemoryRequirements combined_reqs = VulkanBuffer::GetMemoryRequirements(m_logical_device, m_combined_buffer);
		VulkanBuffer::Allocate(
			m_logical_device,
			m_combined_buffer,
			combined_reqs,
			VulkanBuffer::FindMemoryType(
				m_logical_device,
				m_combined_buffer,
				combined_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			)
		);

		// Pack vertex + index information into buffer
		VkDeviceSize vertices_size = sizeof(Vertex) * m_vertices.size();
		VkDeviceSize indices_size = sizeof(uint32_t) * m_indices.size();
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, 0, vertices_size, 0, m_vertices.data());
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, vertices_size, indices_size, 0, m_indices.data());
	}

	// Build Pipelines
	VulkanPipelineFactory pipeline_factory;
	pipeline_factory.Initialize(m_logical_device, renderer->GetVkPipelineBuffer());

	pipeline_factory.Configure<VulkanGraphicsPipeline>()
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/HelloSquare/V-HelloSquare.vert", false))
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/HelloSquare/F-HelloSquare.frag", false))
		.ConfigureVertexInputState()
			.AddVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
			.AddVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
			.AddVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
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

void HelloSquareLayer::Record(const IGraphicsCommand* command)
{
	if (!m_enabled || m_pipeline.handle == VK_NULL_HANDLE)
		return;

	VulkanRenderCommand* render_command = (VulkanRenderCommand*)command;

	// Copy Buffer Data
	VulkanBuffer::Copy(m_logical_device, render_command->graphics_buffer, m_staging_buffer, m_combined_buffer, 0, 0, m_staging_buffer.size);

	// Begin Rendering
	{
		VkRenderingAttachmentInfo color_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = render_command->render_view,
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
				.extent = VkExtent2D{ render_command->render_extent.width, render_command->render_extent.height }
			},
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment
		};

		vkCmdBeginRendering(render_command->graphics_buffer, &render_info);
	}

	vkCmdBindPipeline(render_command->graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(render_command->render_extent.width);
	viewport.height = static_cast<float>(render_command->render_extent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(render_command->graphics_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = render_command->render_extent.width;
	scissor.extent.height = render_command->render_extent.height;

	vkCmdSetScissor(render_command->graphics_buffer, 0, 1, &scissor);

	VkBuffer vertex_buffers[] = { m_combined_buffer.handle };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(render_command->graphics_buffer, 0, 1, vertex_buffers, offsets);

	vkCmdBindIndexBuffer(render_command->graphics_buffer, m_combined_buffer.handle, sizeof(Vertex) * m_vertices.size(), VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(render_command->graphics_buffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

	// End Rendering
	{
		vkCmdEndRendering(render_command->graphics_buffer);
	}
}

void HelloSquareLayer::Enable()
{
	m_enabled = true;
}

void HelloSquareLayer::Disable()
{
	m_enabled = false;
}

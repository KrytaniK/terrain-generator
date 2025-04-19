#include  <macros/AurionLog.h>

#include <chrono>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

import HelloCube;
import Resources;
import Vulkan;

HelloCubeLayer::HelloCubeLayer()
	: m_enabled(true), m_pipeline({})
{
	// Todo: Fix & Label Vertex Data

	// Generate Vertex Data
	m_vertices = {
		{{-1.f, -1.f, 1.f}, {0.0f, 0.0f, 0.75f}},	// 0 + Color Used For Top Face
		{{1.f, -1.f, 1.f}, {0.0f, 0.75f, 0.0f}},	// 1 + Color Used For Back Face 
		{{1.f, 1.f, 1.f}, {0.75f, 0.0f, 0.0f}},		// 2 + Color Used For Right Face
		{{-1.f, 1.f, 1.f}, {0.0f, 0.75f, 0.0f}},	// 3 + Color Used For Front Face
		{{-1.f, -1.f, -1.f}, {0.75f, 0.0f, 0.0f}},	// 4 + Color Used For Left Face
		{{1.f, -1.f, -1.f}, {1.0f, 0.0f, 0.0f}},	// 5 
		{{1.f, 1.f, -1.f}, {0.0f, 0.0f, 1.0f}},		// 6 
		{{-1.f, 1.f, -1.f}, {0.0f, 0.0f, 0.75f}},	// 7 + Color Used For Bottom Face
	};

	// Generate Index Data
	m_indices = {
		// Top face (+Z) (Blue)
		0, 1, 2,
		0, 2, 3,

		// Bottom Face (-Z) (Blue)
		7, 6, 5,
		7, 5, 4,

		// Front Face (+Y)
		3, 2, 6,
		3, 6, 7,

		// Back Face (-Y)
		1, 0, 4,
		1, 4, 5,

		// Right Face (+X)
		2, 1, 5,
		2, 5, 6,

		// Left Face (-X)
		4, 3, 7,
		4, 0, 3
	};
}

HelloCubeLayer::~HelloCubeLayer()
{
	VulkanBuffer::Destroy(m_logical_device, m_staging_buffer);
	VulkanBuffer::Destroy(m_logical_device, m_combined_buffer);

	VulkanDescriptorPool::Destroy(m_logical_device, m_mvp_desc_pool);
	m_mvp_desc_sets.clear();

	VulkanDescriptorSetLayout::Destroy(m_logical_device, m_mvp_desc_layout);

	for (auto& buffer : m_mvp_buffers)
		VulkanBuffer::Destroy(m_logical_device, buffer);
}

void HelloCubeLayer::Initialize(VulkanRenderer* renderer)
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

		// Write vertex data into buffer
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, 0, vertices_size, 0);
		VulkanBuffer::Write(m_staging_buffer, m_vertices.data(), vertices_size);
		VulkanBuffer::UnMap(m_logical_device, m_staging_buffer);

		// Write index data into buffer
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, vertices_size, indices_size, 0);
		VulkanBuffer::Write(m_staging_buffer, m_indices.data(), indices_size);
		VulkanBuffer::UnMap(m_logical_device, m_staging_buffer);
	}

	uint32_t max_frames_in_flight = renderer->GetMaxFramesInFlight();

	// We need enough mvp matrix buffers to handle the number of supported
	//	in-flight frames.
	VkDeviceSize buffer_size = sizeof(ModelViewProjectionMatrix);
	m_mvp_buffers.resize(max_frames_in_flight);
	for (size_t i = 0; i < m_mvp_buffers.size(); i++)
	{
		// Create Buffer
		m_mvp_buffers[i] = VulkanBuffer::Create(m_logical_device, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
		VulkanBuffer& buffer = m_mvp_buffers[i];

		// Allocate Buffer Memory
		VkMemoryRequirements staging_reqs = VulkanBuffer::GetMemoryRequirements(m_logical_device, buffer);
		VulkanBuffer::Allocate(
			m_logical_device,
			buffer,
			staging_reqs,
			VulkanBuffer::FindMemoryType(
				m_logical_device,
				buffer,
				staging_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			)
		);

		// Persistent Map Buffer
		VulkanBuffer::Map(m_logical_device, buffer, 0, buffer_size, 0);
	}

	// Creat Descriptor Set Layout
	m_mvp_desc_layout = VulkanDescriptorSetLayout::Create(m_logical_device, {
		{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }
	});

	// Create Descriptor Pool
	m_mvp_desc_pool = VulkanDescriptorPool::Create(
		m_logical_device,
		max_frames_in_flight,
		{ VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_frames_in_flight } }
	);

	// Allocate Descriptor Sets
	m_mvp_desc_sets = VulkanDescriptorPool::Allocate(
		m_logical_device,
		m_mvp_desc_pool,
		m_mvp_desc_layout,
		max_frames_in_flight
	);

	// Bind MVP buffers to descriptor sets
	std::vector<VkDescriptorBufferInfo> buffer_infos(max_frames_in_flight);
	std::vector<VkWriteDescriptorSet> writes(max_frames_in_flight);
	for (uint32_t i = 0; i < max_frames_in_flight; i++)
	{
		VkDescriptorBufferInfo& buffer_info = buffer_infos[i];
		buffer_info.buffer = m_mvp_buffers[i].handle;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(ModelViewProjectionMatrix);

		VkWriteDescriptorSet& write = writes[i];
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.dstSet = m_mvp_desc_sets[i];
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.pBufferInfo = &buffer_infos[i];
	}

	// Batch update descriptor sets
	vkUpdateDescriptorSets(m_logical_device->handle, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

	// Build Pipelines
	VulkanPipelineFactory pipeline_factory;
	pipeline_factory.Initialize(m_logical_device, renderer->GetVkPipelineBuffer());

	pipeline_factory.Configure<VulkanGraphicsPipeline>()
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/HelloCube/V-HelloCube.vert", false))
		.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/HelloCube/F-HelloCube.frag", false))
		.ConfigurePipelineLayout()
			.AddDescriptorSetLayout(m_mvp_desc_layout.handle)
		.ConfigureVertexInputState()
			.AddVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
			.AddVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
			.AddVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
		.ConfigureInputAssemblyState()
			.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		.ConfigureRasterizationState()
			.SetPolygonMode(VK_POLYGON_MODE_FILL)
			.SetCullMode(VK_CULL_MODE_BACK_BIT)
			.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
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
		.SetDynamicStencilAttachmentFormat(VK_FORMAT_UNDEFINED);

	m_pipeline = pipeline_factory.Build()[0];
}

void HelloCubeLayer::Record(const IGraphicsCommand* command)
{
	if (!m_enabled || m_pipeline.handle == VK_NULL_HANDLE)
		return;

	VulkanRenderCommand* render_command = (VulkanRenderCommand*)command;

	// Copy Buffer Data
	VulkanBuffer::Copy(m_logical_device, render_command->graphics_buffer, m_staging_buffer, m_combined_buffer, 0, 0, m_staging_buffer.size);

	// Update MVP matrix and write to the relevant buffer
	float aspect_ratio = render_command->render_extent.width / ((float)render_command->render_extent.height);
	this->Rotate(glm::radians(45.f), aspect_ratio, 0.1f, 1000.f);
	VulkanBuffer::Write(m_mvp_buffers[render_command->current_frame], &m_mvp_matrix, sizeof(ModelViewProjectionMatrix));

	// Begin Rendering
	{
		VkRenderingAttachmentInfo color_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = render_command->render_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
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

	// Update MVP matrix descriptor set for this frame
	vkCmdBindDescriptorSets(
		render_command->graphics_buffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		m_pipeline.layout, 
		0, 1, 
		&m_mvp_desc_sets[render_command->current_frame], 
		0, nullptr
	);

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

void HelloCubeLayer::Enable()
{
	m_enabled = true;
}

void HelloCubeLayer::Disable()
{
	m_enabled = false;
}

void HelloCubeLayer::Rotate(float fov, float aspect, float near_clip, float far_clip)
{
	// From https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/00_Descriptor_set_layout_and_buffer.html

	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	// Rotate the model around the z-axis at 90-degrees per second
	m_mvp_matrix.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(5.f), glm::vec3(0.0f, 0.0f, 1.f));
	//m_mvp_matrix.model = glm::mat4(1.0f);

	// View the geometry from an angle
	glm::vec3 cam_pos(10.f, 10.f, 5.f);
	glm::vec3 obj_pos(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.f, 0.f, 1.f);
	m_mvp_matrix.view = glm::lookAt(cam_pos, obj_pos, up);

	// Setup projection
	m_mvp_matrix.projection = glm::perspective(fov, aspect, near_clip, far_clip);

	m_mvp_matrix.projection[1][1] *= -1;
}

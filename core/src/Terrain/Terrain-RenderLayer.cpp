#include <macros/AurionLog.h>

#include <functional>

#include <vulkan/vulkan.h>
#include <glm/gtc/matrix_transform.hpp>

import Resources;
import World;
import Terrain;
import Vulkan;

TerrainRenderLayer::TerrainRenderLayer()
{
	m_enabled = true;
	m_revalidate_terrain = true;
	m_vertex_count = 0;
	m_index_count = 0;
}

TerrainRenderLayer::~TerrainRenderLayer()
{
	VulkanBuffer::Destroy(m_logical_device, m_staging_buffer);
	VulkanBuffer::Destroy(m_logical_device, m_terrain_buffer);

	VulkanDescriptorPool::Destroy(m_logical_device, m_mvp_desc_pool);
	m_mvp_desc_sets.clear();

	VulkanDescriptorSetLayout::Destroy(m_logical_device, m_mvp_desc_layout);

	for (auto& buffer : m_mvp_buffers)
		VulkanBuffer::Destroy(m_logical_device, buffer);
}

void TerrainRenderLayer::Initialize(VulkanRenderer* renderer, World& world, TerrainEventDispatcher& event_dispatcher)
{
	m_renderer = renderer;
	m_logical_device = renderer->GetLogicalDevice();
	m_world = &world;
	m_camera = &m_world->GetPrimaryCamera();

	m_terrain_listener.Bind([this]() {
		m_revalidate_terrain = true;
	});
	event_dispatcher.AddEventListener(&m_terrain_listener);

	// Generate Camera Resources
	this->GenerateCameraBuffers();

	// Build Pipelines
	this->BuildTerrainPipelines();
}

void TerrainRenderLayer::Record(const IGraphicsCommand* command)
{
	if (!m_enabled || m_wireframe_pipeline.handle == VK_NULL_HANDLE)
		return;

	VulkanCommand* render_command = (VulkanCommand*)command;

	// Copy Buffer Data
	if (m_revalidate_terrain)
	{
		// Re-generate CPU/GPU buffers with new sizes
		this->GenerateTerrainBuffers();

		// Don't copy if there's nothing to render
		if (m_vertex_count == 0)
			return;

		// Copy the staging buffer into the rendered buffer, then immediately invalidate terrain data
		VulkanBuffer::Copy(m_logical_device, render_command->graphics_buffer, m_staging_buffer, m_terrain_buffer, 0, 0, m_staging_buffer.size);
		m_revalidate_terrain = false;
	}

	// Update MVP matrix and write to the relevant buffer
	m_camera->aspect_ratio = render_command->color_image.extent.width / ((float)render_command->color_image.extent.height);
	m_mvp_matrix.model = glm::mat4(1.0f);
	m_mvp_matrix.view = m_camera->view_projection.view;
	m_mvp_matrix.projection = m_camera->view_projection.projection;
	VulkanBuffer::Write(m_mvp_buffers[render_command->current_frame], &m_mvp_matrix, sizeof(ModelViewProjectionMatrix));

	// Begin Rendering
	{
		VkRenderingAttachmentInfo color_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = render_command->color_image.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
			.resolveImageView = render_command->resolve_image.view,
			.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = VkClearValue{
				.color = {
					0.125f,
					0.125f,
					0.125f,
					1.0f
				}
			}
		};

		VkRenderingAttachmentInfo depth_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = render_command->depth_image.view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.depthStencil = { 1.0f, 0 }
			}
		};

		VkRenderingInfo render_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = VkRect2D{
				.extent = VkExtent2D{ render_command->color_image.extent.width, render_command->color_image.extent.height }
			},
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment,
			.pDepthAttachment = &depth_attachment,
		};

		vkCmdBeginRendering(render_command->graphics_buffer, &render_info);

		// Bind relevant pipeline
		VulkanPipeline& pipeline = m_wireframe_pipeline;
		vkCmdBindPipeline(render_command->graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);

		// Update MVP matrix descriptor set for this frame
		vkCmdBindDescriptorSets(
			render_command->graphics_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline.layout,
			0, 1,
			&m_mvp_desc_sets[render_command->current_frame],
			0, nullptr
		);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(render_command->color_image.extent.width);
		viewport.height = static_cast<float>(render_command->color_image.extent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(render_command->graphics_buffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = render_command->color_image.extent.width;
		scissor.extent.height = render_command->color_image.extent.height;

		vkCmdSetScissor(render_command->graphics_buffer, 0, 1, &scissor);

		if (m_terrain_buffer.handle != VK_NULL_HANDLE)
		{
			VkBuffer vertex_buffers[] = { m_terrain_buffer.handle };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(render_command->graphics_buffer, 0, 1, vertex_buffers, offsets);

			//vkCmdDraw(render_command->graphics_buffer, m_vertex_count, 1, 0, 0);

			vkCmdBindIndexBuffer(render_command->graphics_buffer, m_terrain_buffer.handle, sizeof(Vertex) * m_vertex_count, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(render_command->graphics_buffer, static_cast<uint32_t>(m_index_count), 1, 0, 0, 0);
		}

		vkCmdEndRendering(render_command->graphics_buffer);
	}
}

void TerrainRenderLayer::Enable()
{
	m_enabled = true;
}

void TerrainRenderLayer::Disable()
{
	m_enabled = false;
}

void TerrainRenderLayer::GenerateTerrainBuffers()
{
	if (!m_world)
		return;

	// Figure out how large the buffer needs to be
	WorldPartition& root =  m_world->GetRootPartition();

	// Traverse the world tree to get vertex & index counts
	root.Traverse([&](WorldPartition& part) {
		TerrainChunk& terrain = part.GetTerrain();
		
		m_vertex_count += terrain.vertices.size();
		m_index_count += terrain.indices.size();
	});

	// Bail if there's no vertices to copy
	if (m_vertex_count == 0)
		return;

	// Calculate vertex and index sizes in buffer
	VkDeviceSize vertices_size = sizeof(Vertex) * m_vertex_count;
	VkDeviceSize indices_size = sizeof(uint32_t) * m_index_count;
	size_t buffer_size = vertices_size + indices_size;

	// Resize buffers if necessary
	if (buffer_size > m_staging_buffer.size)
	{
		// Destroy Old Buffers
		vkDeviceWaitIdle(m_logical_device->handle);
		if (m_staging_buffer.handle != VK_NULL_HANDLE)
			VulkanBuffer::Destroy(m_logical_device, m_staging_buffer);
		if (m_terrain_buffer.handle != VK_NULL_HANDLE)
			VulkanBuffer::Destroy(m_logical_device, m_terrain_buffer);

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
		m_terrain_buffer = VulkanBuffer::Create(
			m_logical_device,
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_SHARING_MODE_EXCLUSIVE
		);

		VkMemoryRequirements combined_reqs = VulkanBuffer::GetMemoryRequirements(m_logical_device, m_terrain_buffer);
		VulkanBuffer::Allocate(
			m_logical_device,
			m_terrain_buffer,
			combined_reqs,
			VulkanBuffer::FindMemoryType(
				m_logical_device,
				m_terrain_buffer,
				combined_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			)
		);
	}

	// Traverse the world tree once more, copying all vertex & index data into the buffer
	size_t vertex_offset = 0;
	size_t index_offset = vertices_size;
	root.Traverse([&](WorldPartition& part) {
		TerrainChunk& terrain = part.GetTerrain();

		// Copy terrain indices, as they'll be modified.
		//	Indices need to be offset to account for vertex packing
		std::vector<uint32_t> indices(terrain.indices.size());
		for (size_t i = 0; i < indices.size(); i++)
			indices[i] = terrain.indices[i] + (uint32_t)vertex_offset;

		// Write Vertex data into staging buffer
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, vertex_offset, terrain.vertices.size(), 0);
		VulkanBuffer::Write(m_staging_buffer, terrain.vertices.data(), sizeof(Vertex) * terrain.vertices.size());
		VulkanBuffer::UnMap(m_logical_device, m_staging_buffer);

		// Write Index data into staging buffer
		VulkanBuffer::Map(m_logical_device, m_staging_buffer, index_offset, indices.size(), 0);
		VulkanBuffer::Write(m_staging_buffer, indices.data(), sizeof(uint32_t) * indices.size());
		VulkanBuffer::UnMap(m_logical_device, m_staging_buffer);
		
		// Update vertex & index offsets
		vertex_offset += terrain.vertices.size();
		index_offset += terrain.indices.size();
	});
}

void TerrainRenderLayer::GenerateCameraBuffers()
{
	uint32_t max_frames_in_flight = m_renderer->GetMaxFramesInFlight();

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
}

void TerrainRenderLayer::BuildTerrainPipelines()
{
	VulkanPipelineFactory pipeline_factory;
	pipeline_factory.Initialize(m_logical_device, m_renderer->GetVkPipelineBuffer());

	pipeline_factory.Configure<VulkanGraphicsPipeline>()
		.BindShader(Vulkan::CreatePipelineShader(m_logical_device, VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/terrain/v-terrain.vert", false))
		.BindShader(Vulkan::CreatePipelineShader(m_logical_device, VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/terrain/f-terrain.frag", false))
		.ConfigurePipelineLayout()
		.AddDescriptorSetLayout(m_mvp_desc_layout.handle)
		.ConfigureVertexInputState()
		.AddVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
		.AddVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
		.AddVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
		.ConfigureInputAssemblyState()
		.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
		.ConfigureRasterizationState()
		.SetPolygonMode(VK_POLYGON_MODE_FILL)
		.SetCullMode(VK_CULL_MODE_NONE)
		.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.SetLineWidth(1.0f)
		.ConfigureColorBlendState()
		.SetLogicOpEnabled(VK_FALSE)
		.SetBlendConstants(0.f, 0.f, 0.f, 0.f)
		.AddColorAttachment()
		.SetBlendEnabled(VK_FALSE)
		.SetSrcColorBlendFactor(VK_BLEND_FACTOR_SRC_COLOR)
		.SetDstColorBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)
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
		.SetRasterizationSamples(VK_SAMPLE_COUNT_4_BIT)
		.SetMinSampleShading(1.0f)
		.SetAlphaToCoverageEnabled(VK_FALSE)
		.SetAlphaToOneEnabled(VK_FALSE)
		.ConfigureDepthStencilState()
		.SetDepthTestEnabled(VK_FALSE)
		.SetDepthWriteEnabled(VK_FALSE)
		.SetDepthCompareOp(VK_COMPARE_OP_LESS)
		.SetMinDepthBounds(0.0f)
		.SetMaxDepthBounds(1.0f)
		.SetDepthBoundsTestEnabled(VK_FALSE)
		.ConfigureDynamicState()
		.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
		.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.AddDynamicColorAttachmentFormat(VK_FORMAT_B8G8R8A8_UNORM)
		.SetDynamicDepthAttachmentFormat(VK_FORMAT_D32_SFLOAT_S8_UINT)
		.SetDynamicStencilAttachmentFormat(VK_FORMAT_UNDEFINED);

	std::span<VulkanPipeline> pipelines = pipeline_factory.Build();
	m_wireframe_pipeline = pipelines[0];
}

void TerrainRenderLayer::Rotate(float fov, float aspect, float near_clip, float far_clip)
{
	// From https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/00_Descriptor_set_layout_and_buffer.html

	m_mvp_matrix.model = glm::mat4(1.0f);

	// View the geometry from an angle
	glm::vec3 cam_pos(-10.f, -10.f, 5.f);
	glm::vec3 obj_pos(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.f, 0.f, 1.f);
	m_mvp_matrix.view = glm::lookAt(cam_pos, obj_pos, up);

	// Setup projection
	m_mvp_matrix.projection = glm::perspective(fov, aspect, near_clip, far_clip);

	m_mvp_matrix.projection[1][1] *= -1;
}
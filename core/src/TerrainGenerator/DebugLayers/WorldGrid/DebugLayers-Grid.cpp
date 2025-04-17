#include <macros/AurionLog.h>

#include <chrono>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

import DebugLayers;
import Vulkan;

DebugGridLayer::DebugGridLayer()
	: m_enabled(true)
{

}

DebugGridLayer::~DebugGridLayer()
{
	VulkanImage::Destroy(m_logical_device, m_msaa_image);

	VulkanDescriptorPool::Destroy(m_logical_device, m_mvp_desc_pool);
	m_mvp_desc_sets.clear();

	VulkanDescriptorSetLayout::Destroy(m_logical_device, m_mvp_desc_layout);

	for (auto& buffer : m_mvp_buffers)
		VulkanBuffer::Destroy(m_logical_device, buffer);
}

void DebugGridLayer::Initialize(const DebugGridConfig* config, VulkanRenderer* renderer, const Aurion::WindowHandle& window_handle)
{
	m_logical_device = renderer->GetLogicalDevice();
	m_config = config;

	// Generate the MVP matrix buffers
	{
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
	}

	// Setup Push Constants
	m_config_pc.offset = 0;
	m_config_pc.size = sizeof(DebugGridConfig);
	m_config_pc.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	// Create MSAA image
	VkPhysicalDeviceLimits limits = m_logical_device->properties.properties.limits;
	if (!(limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_8_BIT))
	{
		AURION_ERROR("Physical Device does not support a sample count of 8");
		return;
	}

	VulkanImageCreateInfo create_info{};
	create_info.extent = VkExtent3D{
		.width = window_handle.window->GetWidth(),
		.height = window_handle.window->GetHeight(),
		.depth = 1
	};

	// Ensure each image matches the swapchain format
	create_info.format = VK_FORMAT_B8G8R8A8_UNORM;

	create_info.usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.usage_flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

	create_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

	// Enable 4x MSAA by default
	create_info.msaa_samples = VK_SAMPLE_COUNT_8_BIT;

	m_msaa_image = VulkanImage::Create(m_logical_device->handle, m_logical_device->allocator, create_info);

	// Build Pipeline
	{
		VulkanPipelineFactory pipeline_factory;
		pipeline_factory.Initialize(m_logical_device, renderer->GetVkPipelineBuffer());

		pipeline_factory.Configure<VulkanGraphicsPipeline>()
			.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/DebugGrid/V-DebugGrid.vert", false))
			.BindShader(Vulkan::CreatePipelineShader(renderer->GetLogicalDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/DebugGrid/F-DebugGrid.frag", false))
			.ConfigurePipelineLayout()
				.AddDescriptorSetLayout(m_mvp_desc_layout.handle)
				.AddPushConstantRange(m_config_pc)
			.ConfigureVertexInputState()
			.ConfigureInputAssemblyState()
			.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.ConfigureRasterizationState()
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.SetCullMode(VK_CULL_MODE_NONE)
				.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetLineWidth(1.0f)
			.ConfigureColorBlendState()
				.SetLogicOpEnabled(VK_FALSE)
				.SetBlendConstants(0.f, 0.f, 0.f, 0.f)
				.AddColorAttachment()
					.SetBlendEnabled(VK_TRUE)
					.SetSrcColorBlendFactor(VK_BLEND_FACTOR_SRC_COLOR)
					.SetDstColorBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)
					.SetColorBlendOp(VK_BLEND_OP_ADD)
					.SetSrcAlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA)
					.SetDstAlphaBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
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

		m_grid_pipeline = pipeline_factory.Build()[0];
	}
}

void DebugGridLayer::Record(const IGraphicsCommand* command)
{
	VulkanRenderCommand* cmd = (VulkanRenderCommand*)command;

	//if (cmd->render_extent.width != m_msaa_image.extent.width || cmd->render_extent.height != m_msaa_image.extent.height)
		//this->RevalidateImage(cmd->render_extent);


	// Update View based on window size
	float aspect_ratio = cmd->render_extent.width / ((float)cmd->render_extent.height);
	this->UpdateViewMatrix(glm::radians(45.f), aspect_ratio, 0.1f, 1000.f);
	VulkanBuffer::Write(m_mvp_buffers[cmd->current_frame], &m_mvp_matrix, sizeof(ModelViewProjectionMatrix));

	/*VulkanImage::TransitionLayouts(cmd->graphics_buffer, {
		VulkanImage::CreateLayoutTransition(
			m_msaa_image.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
		)
	});*/

	// Begin Rendering
	{
		VkRenderingAttachmentInfo color_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = cmd->render_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			//.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
			//.resolveImageView = cmd->render_view,
			//.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = VkClearValue{
				.color = VkClearColorValue{
					0.15f,
					0.15f,
					0.15f,
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
	}

	// Dynamic Viewport State
	{
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
	}

	vkCmdBindPipeline(cmd->graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_grid_pipeline.handle);

	// Update MVP matrix descriptor set for this frame
	vkCmdBindDescriptorSets(
		cmd->graphics_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_grid_pipeline.layout,
		0, 1,
		&m_mvp_desc_sets[cmd->current_frame],
		0, nullptr
	);

	// Update Push Constants
	vkCmdPushConstants(cmd->graphics_buffer, m_grid_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DebugGridConfig), m_config);

	vkCmdDraw(cmd->graphics_buffer, 6, 1, 0, 0);

	// End Rendering
	{
		vkCmdEndRendering(cmd->graphics_buffer);
	}
}

void DebugGridLayer::Enable()
{
	m_enabled = true;
}

void DebugGridLayer::Disable()
{
	m_enabled = false;
}

void DebugGridLayer::UpdateViewMatrix(float fov, float aspect, float near_clip, float far_clip)
{
	// From https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/00_Descriptor_set_layout_and_buffer.html

	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	// Rotate the model around the z-axis at 90-degrees per second
	m_mvp_matrix.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(5.f), glm::vec3(0.0f, 0.0f, 1.f));
	//m_mvp_matrix.model = glm::mat4(1.0f);

	glm::vec3 cam_pos(0.1f, 10.f, 1.f);
	glm::vec3 obj_pos(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.f, 0.f, 1.f);
	m_mvp_matrix.view = glm::lookAt(cam_pos, obj_pos, up);

	m_mvp_matrix.projection = glm::perspective(
		fov,
		aspect,
		near_clip,
		far_clip
	);

	m_mvp_matrix.projection[1][1] *= -1;
}

void DebugGridLayer::RevalidateImage(const VkExtent3D& new_extent)
{
	vkDeviceWaitIdle(m_logical_device->handle);

	// Destroy old image data
	{
		vkDestroySampler(m_logical_device->handle, m_msaa_image.sampler, nullptr);
		vkDestroyImageView(m_logical_device->handle, m_msaa_image.view, nullptr);
		vmaDestroyImage(m_logical_device->allocator, m_msaa_image.image, m_msaa_image.allocation);
	}

	VulkanImageCreateInfo create_info{};
	create_info.extent = VkExtent3D{
		.width = new_extent.width,
		.height = new_extent.height,
		.depth = 1
	};

	// Ensure each image matches the swapchain format
	create_info.format = VK_FORMAT_B8G8R8A8_UNORM;

	create_info.usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.usage_flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

	create_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

	create_info.msaa_samples = VK_SAMPLE_COUNT_8_BIT;

	m_msaa_image = VulkanImage::Create(m_logical_device->handle, m_logical_device->allocator, create_info);
}

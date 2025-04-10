#include <macros/AurionLog.h>

#include <functional>
#include <cmath>
#include <chrono>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import TerrainGenerator;
import Aurion.GLFW;
import Vulkan;

TerrainGenerator::TerrainGenerator()
{
	
}

TerrainGenerator::~TerrainGenerator()
{

}

void TerrainGenerator::StartAndRun()
{
	this->Load();
	this->Start();
	this->Run();
	this->Unload();
}

void TerrainGenerator::Load()
{
	// Potentially load config from file
	Aurion::WindowDriverConfig driver_config{};
	driver_config.max_window_count = 10;

	m_window_driver.Initialize(driver_config);

	VulkanDriverConfiguration vk_driver_config{};
	vk_driver_config.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_driver_config.app_info.pApplicationName = "Terrain Generator";
	vk_driver_config.app_info.pEngineName = "No Engine";
	vk_driver_config.app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_driver_config.app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 309);
	vk_driver_config.enable_validation_layers = false;
	vk_driver_config.enable_debug_messenger = false;
	vk_driver_config.max_frames_in_flight = 3;
	vk_driver_config.validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	// Core Vulkan Features
	VkPhysicalDeviceFeatures features{};
	features.geometryShader = VK_TRUE;
	features.tessellationShader = VK_TRUE;

	// Vulkan 1.1 Features
	VkPhysicalDeviceVulkan11Features features11{};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	// Vulkan 1.2 Features
	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	features12.pNext = &features11;

	// Vulkan 1.3 Features
	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	features13.pNext = &features12;

	// Vulkan 1.4 Features
	VkPhysicalDeviceVulkan14Features features14{};
	features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	features14.pNext = &features13;

	// Package core and new features
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = features;
	deviceFeatures2.pNext = &features14;

	// VMA Allocator Flags
	VmaAllocatorCreateFlags allocator_flags = 0;

	// Package features/properties/flags/extensions into device requirements
	VulkanDeviceConfiguration vk_device_config{};
	vk_device_config.features = deviceFeatures2;
	vk_device_config.device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU; // Prefer dedicated GPU
	vk_device_config.allocator_flags = allocator_flags;
	vk_device_config.extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};
	vk_device_config.layers = vk_driver_config.validation_layers;

	// Potentially load vulkan driver config from file
	m_vulkan_driver.SetConfiguration(vk_driver_config);
	m_vulkan_driver.SetDeviceConfiguration(vk_device_config);
	m_vulkan_driver.Initialize();

	m_renderer = (VulkanRenderer*)m_vulkan_driver.CreateRenderer();

	// Setting up Hello Triangle
	VulkanPipelineBuilder builder;
	builder.Initialize(m_renderer->GetLogicalDevice(), m_renderer->GetVkPipelineBuffer());
	
	// Configuring the Graphics Pipeline
	{
		// Graphics Pipeline Configuration (Based On Vulkan Tutorial: https://vulkan-tutorial.com/ and the Vulkan Guide: https://vkguide.dev/)
		builder.Configure(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
		.UseDynamicRendering()
			.AddDynamicColorAttachmentFormat(VK_FORMAT_B8G8R8A8_UNORM)
			.SetDynamicDepthAttachmentFormat(VK_FORMAT_D32_SFLOAT)
			.SetDynamicStencilAttachmentFormat(VK_FORMAT_UNDEFINED)
		.BindShader(VK_SHADER_STAGE_VERTEX_BIT, 0, "assets/shaders/vert-shader.vert", false)
		.BindShader(VK_SHADER_STAGE_FRAGMENT_BIT, 0, "assets/shaders/frag-shader.frag", false)
		.ConfigurePipelineLayout() // Pipeline Layout
			// Currently Unused
		.BuildPipelineLayout()
		.ConfigureVertexInputState() // Vertex Input State
			// None for now
		.BuildVertexInputState()
		.ConfigureInputAssemblyState() // Input Assembly State
			.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.SetPrimitiveRestartEnable(VK_FALSE)
		.ConfigureTessellationState() // Tessellation State
		.ConfigureViewportState()
			.AddViewport(VkViewport{})
			.AddScissor(VkRect2D{})
		.BuildViewportState()
		.ConfigureRasterizationState() // Rasterization State
			.SetDepthClampEnabled(VK_FALSE)
			.SetRasterizerDiscardEnabled(VK_FALSE)
			.SetPolygonMode(VK_POLYGON_MODE_FILL)
			.SetLineWidth(1.0f)
			.SetCullMode(VK_CULL_MODE_BACK_BIT)
			.SetFrontFace(VK_FRONT_FACE_CLOCKWISE)
			.SetDepthBiasEnabled(VK_FALSE)
			.SetDepthBiasConstantFactor(0.0f)
			.SetDepthBiasClamp(0.0f)
			.SetDepthBiasSlopeFactor(0.0f)
		.ConfigureMultisampleState() // MultisampleState
			.SetSampleShadingEnabled(VK_FALSE)
			.SetRasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
			.SetMinSampleShading(1.0f)
			.SetAlphaToCoverageEnabled(VK_FALSE)
			.SetAlphaToOneEnabled(VK_FALSE)
		.BuildMultisampleState()
		.ConfigureDepthStencilState() // Depth Stencil State
			// Unused
		.ConfigureColorBlendState() // Color Blend State
			.AddColorAttachment()
				.SetColorWriteMask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
				.SetBlendEnabled(VK_FALSE)
				.SetSrcColorBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetDstColorBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetColorBlendOp(VK_BLEND_OP_ADD)
				.SetSrcAlphaBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetDstAlphaBlendFactor(VK_BLEND_FACTOR_ONE)
				.SetAlphaBlendOp(VK_BLEND_OP_ADD)
			.SetLogicOpEnabled(VK_FALSE)
			.SetLogicOp(VK_LOGIC_OP_COPY)
			.SetBlendConstants(0.0f, 0.0f, 0.0f, 0.0f)
		.BuildColorBlendState()
		.ConfigureDynamicState() // Dynamic State
			.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
			.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
		.BuildDynamicState();
	}

	// Building all pipelines
	m_render_pipelines = builder.Build();
}

void TerrainGenerator::Start()
{
	m_should_close = false;

	Aurion::WindowConfig window_config;
	window_config.title = "Terrain Generator";

	// Create the main window
	Aurion::WindowHandle main_window = m_window_driver.InitWindow(window_config);

	// Create a graphics context for that window
	VulkanContext* ctx = m_renderer->CreateContext(main_window);

	// Submit a single command to process the rendering for this window
	ctx->BindRenderCommand(std::bind(&TerrainGenerator::Render, this, std::placeholders::_1));
}

void TerrainGenerator::Run()
{
	Aurion::WindowHandle main_window = m_window_driver.GetWindow("Terrain Generator");
	VulkanContext* main_render_context = m_renderer->GetContext(main_window.id);

	while (!m_should_close)
	{
		// Input Polling and Window Updates
		main_window.window->Update();

		main_render_context->RenderFrame();

		// Close if the main window is no longer open
		m_should_close = !main_window.window->IsOpen();
	}
}

void TerrainGenerator::Unload()
{

}

void TerrainGenerator::Render(const VulkanCommand& command)
{
	VulkanPipeline* pipeline = m_render_pipelines.graphics_pipelines[0];

	// Draw Triangle
	VkRenderingAttachmentInfo color_attachment{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = command.render_view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE
	};

	VkRenderingInfo render_info{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = VkRect2D{
			.extent = VkExtent2D{ command.render_extent.width, command.render_extent.height }
		},
		.layerCount = 1,
		.viewMask = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment
	};

	vkCmdBeginRendering(command.graphics_buffer, &render_info);

	vkCmdBindPipeline(command.graphics_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = command.render_extent.width;
	viewport.height = command.render_extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(command.graphics_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = command.render_extent.width;
	scissor.extent.height = command.render_extent.height;

	vkCmdSetScissor(command.graphics_buffer, 0, 1, &scissor);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(command.graphics_buffer, 3, 1, 0, 0);

	vkCmdEndRendering(command.graphics_buffer);
}

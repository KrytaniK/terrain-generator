#include <macros/AurionLog.h>

#include <functional>
#include <cmath>
#include <chrono>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

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

	// Potentially load vulkan driver config from file
	m_vulkan_driver.Initialize();

	m_renderer = (VulkanRenderer*)m_vulkan_driver.CreateRenderer();

	// Setting up Hello Triangle
	VulkanPipelineBuilder* builder = m_renderer->GetPipelineBuilder();
	
	// Configuring the Graphics Pipeline
	{
		// Graphics Pipeline Configuration (Based On Vulkan Tutorial: https://vulkan-tutorial.com/ and the Vulkan Guide: https://vkguide.dev/)
		builder->Configure(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
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
	m_render_pipelines = builder->Build();
}

void TerrainGenerator::Start()
{
	m_should_close = false;

	Aurion::WindowConfig window_config;
	window_config.title = "Terrain Generator";

	// Create the main window
	Aurion::WindowHandle main_window = m_window_driver.InitWindow(window_config);

	// Create a graphics context for that window
	m_renderer->AddWindow(main_window);

	// Submit a single command to process the rendering for this window
	m_renderer->BindCommand(main_window, std::bind(&TerrainGenerator::Render, this, std::placeholders::_1));
}

void TerrainGenerator::Run()
{
	Aurion::WindowHandle main_window = m_window_driver.GetWindow("Terrain Generator");

	while (!m_should_close)
	{
		// Input Polling and Window Updates
		main_window.window->Update();

		// Render Frame
		m_renderer->BeginFrame();
		m_renderer->EndFrame();

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

	// Draw background
	float flash = std::abs(std::sin((static_cast<double>(command.current_frame)) / 120.f));
	VkClearColorValue clear_value{
		0.0f,
		0.0f,
		flash,
		1.0f
	};
	VkImageSubresourceRange clear_range{};
	clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clear_range.baseMipLevel = 0;
	clear_range.levelCount = VK_REMAINING_MIP_LEVELS;
	clear_range.baseArrayLayer = 0;
	clear_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdClearColorImage(command.graphics_buffer, command.render_image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

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

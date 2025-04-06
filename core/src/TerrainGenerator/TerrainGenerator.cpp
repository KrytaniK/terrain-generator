#include <macros/AurionLog.h>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

import TerrainGenerator;
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

	// TESTING PIPELINE BUILDER
	VulkanPipelineBuilder* builder = m_renderer->GetPipelineBuilder();

	builder->
		// Compute Pipeline Configuration
		Configure(VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO)
			.BindShader(VK_SHADER_STAGE_COMPUTE_BIT, 0, "assets/shaders/compute.hlsl", true)
			.ConfigurePipelineLayout()
			.BuildPipelineLayout()
		// Graphics Pipeline Configuration (Based On Vulkan Tutorial: https://vulkan-tutorial.com/)
		.Configure(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
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

	VulkanPipelineBuilder::Result result = builder->Build();
}

void TerrainGenerator::Start()
{
	Aurion::WindowConfig window_config;
	window_config.title = "Terrain Generator";

	// Create the main window
	Aurion::WindowHandle main_window = m_window_driver.InitWindow(window_config);

	m_renderer->AddWindow(main_window);
}

void TerrainGenerator::Run()
{
	m_should_close = false;

	Aurion::IWindow* a = m_window_driver.GetWindow("Terrain Generator").window;
	while (!m_should_close)
	{
		m_renderer->BeginFrame();
		m_renderer->EndFrame();

		m_should_close = !a->IsOpen();
	}
}

void TerrainGenerator::Unload()
{

}
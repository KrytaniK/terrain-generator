#include <macros/AurionLog.h>

#include <functional>
#include <span>
#include <memory>

#include <chrono>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <imgui.h>

import TerrainGenerator;
import HelloTriangle;
import HelloSquare;
import HelloCube;
import ImGuiDemo;

import Aurion.GLFW;
import Graphics;
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
	
	// Vulkan Driver Configuration
	VulkanDriverConfiguration vk_driver_config{};
	vk_driver_config.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_driver_config.app_info.pApplicationName = "Terrain Generator";
	vk_driver_config.app_info.pEngineName = "No Engine";
	vk_driver_config.app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_driver_config.app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 309);
	vk_driver_config.enable_validation_layers = true;
	vk_driver_config.enable_debug_messenger = true;
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

	// Package features/properties/flags/extensions into device requirements
	VulkanDeviceConfiguration vk_device_config{};
	vk_device_config.features = deviceFeatures2;
	vk_device_config.device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU; // Prefer dedicated GPU
	vk_device_config.allocator_flags = 0;
	vk_device_config.extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};

	if (vk_driver_config.enable_validation_layers)
		vk_device_config.layers = vk_driver_config.validation_layers;

	m_vulkan_driver.SetConfiguration(vk_driver_config);
	m_vulkan_driver.SetDeviceConfiguration(vk_device_config);
	m_vulkan_driver.Initialize();

	m_renderer = (VulkanRenderer*)m_vulkan_driver.CreateRenderer();
}

void TerrainGenerator::Start()
{
	m_should_close = false;

	Aurion::WindowConfig window_config;

	// Hello Cube Window
	{
		// GLFW window for Hello Cube
		window_config.title = "Hello Cube";
		Aurion::WindowHandle hello_cube = m_window_driver.InitWindow(window_config);

		// Generate Graphics Context
		VulkanContext* hello_cube_ctx = m_renderer->CreateContext(hello_cube);
		hello_cube_ctx->SetVSyncEnabled(false);

		// Add Render Layer
		HelloCubeLayer* hello_cube_layer = hello_cube_ctx->AddRenderLayer<HelloCubeLayer>();
		hello_cube_layer->Initialize(m_renderer);
	}
}

void TerrainGenerator::Run()
{
	std::vector<Aurion::WindowHandle> windows = {
		m_window_driver.GetWindow("Hello Cube")
	};

	while (!m_should_close)
	{
		// Input Polling and Window Updates
		for (auto& [id, handle] : windows)
			handle->Update();

		// Render Commands
		m_renderer->Render();

		// Close if there are no windows open
		m_should_close = true;
		for (auto& [id, handle] : windows)
			if (handle->IsOpen())
				m_should_close = false;
	}
}

void TerrainGenerator::Unload()
{

}

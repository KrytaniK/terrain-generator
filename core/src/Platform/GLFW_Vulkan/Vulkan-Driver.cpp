#define VMA_IMPLEMENTATION

#include <macros/AurionLog.h>

#include <vector>
#include <span>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanDriver::VulkanDriver()
	: m_config({}), m_vk_instance(VK_NULL_HANDLE), m_vk_debug_messenger(VK_NULL_HANDLE)
{
	// Set up default driver configuration
	{
		// Basic Application Info
		m_config.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		m_config.app_info.pApplicationName = "Terrain Generator";
		m_config.app_info.pEngineName = "No Engine";
		m_config.app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		m_config.app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 309);

		// Always enable validation and debug messenging by default
		m_config.enable_validation_layers = true;
		m_config.enable_debug_messenger = true;

		// Default validation layers
		m_config.validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		// Default to 3 frames in flight
		m_config.max_frames_in_flight = 3;
	}
}

VulkanDriver::~VulkanDriver()
{
	// Make sure all renderers are cleaned up before destroying the instance
	m_renderers.clear();

	if (m_config.enable_debug_messenger)
		Vulkan_DestroyDebugUtilsMessengerEXT(m_vk_instance, m_vk_debug_messenger, nullptr);

	vkDestroyInstance(m_vk_instance, nullptr);
}

const uint64_t VulkanDriver::GetType()
{
	return 0;
}

void VulkanDriver::Initialize()
{
	if (m_vk_instance != VK_NULL_HANDLE)
	{
		AURION_WARN("[Vulkan Driver] Cannot initialize driver. VkInstance already exists!");
		return;
	}

	this->CreateVkInstance();
	this->CreateVkDebugMessenger();
}

void VulkanDriver::Initialize(const VulkanDriverConfiguration& config)
{
	if (m_vk_instance != VK_NULL_HANDLE)
	{
		AURION_WARN("[Vulkan Driver] Cannot initialize driver. VkInstance already exists!");
		return;
	}

	m_config = std::move(config);

	this->CreateVkInstance();
	this->CreateVkDebugMessenger();
}

IRenderer* VulkanDriver::CreateRenderer(const std::vector<Aurion::WindowHandle>& windows)
{
	// Create the renderer
	m_renderers.emplace_back(VulkanRenderer());
	VulkanRenderer& renderer = m_renderers.back();
	
	// Initialize a renderer with the default device requirements
	{
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
		VulkanDeviceRequirements device_reqs{};
		device_reqs.features = deviceFeatures2;
		device_reqs.device_type = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU; // Prefer dedicated GPU
		device_reqs.allocator_flags = allocator_flags;
		device_reqs.extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
		};
		device_reqs.layers = m_config.validation_layers;

		// Initialize renderer with vulkan instance and default device requirements
		renderer.Init(m_vk_instance, device_reqs, m_config.max_frames_in_flight);
	}

	// Attach all provided windows
	for (const auto& handle : windows)
		renderer.AddWindow(handle);

	// return reference to renderer
	return &renderer;
}

IRenderer* VulkanDriver::CreateRenderer(std::span<Aurion::WindowHandle> windows, const VulkanDeviceRequirements& device_reqs)
{
	// Create the renderer
	m_renderers.emplace_back(VulkanRenderer());
	VulkanRenderer& renderer = m_renderers.back();

	// Initialize renderer with vulkan instance and default device requirements
	renderer.Init(m_vk_instance, device_reqs, m_config.max_frames_in_flight);

	// Attach all provided windows
	for (const auto& handle : windows)
		renderer.AddWindow(handle);

	// return reference to renderer
	return &renderer;
}

void VulkanDriver::CreateVkInstance()
{
	// Ensure validation layer support
	if (m_config.enable_validation_layers)
	{
		// Query for the number of instance layers
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		// Query for all layer properties
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Ensure all requested layers are available
		for (const char* validationLayer : m_config.validation_layers)
		{
			bool found = false;

			// Find validation layer in instance layers
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(validationLayer, layerProperties.layerName) == 0)
				{
					found = true;
					break;
				}
			}

			// return false if this validation layer wasn't found
			if (!found)
			{
				AURION_CRITICAL(
					"[Vulkan Backend] One or more requested validation layers are not available!\nFirst is: %s",
					validationLayer
				);
				return;
			}
		}
	}

	// Instance Create info
	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &m_config.app_info;

	// Debug Messenger Setup
	if (m_config.enable_debug_messenger)
	{
		// Setup Debug Messenger
		m_config.debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		m_config.debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		m_config.debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		m_config.debug_messenger_info.pfnUserCallback = Vulkan_DebugCallback;
		m_config.debug_messenger_info.pUserData = nullptr;

		// Enable Validation Layers
		instance_info.enabledLayerCount = static_cast<uint32_t>(m_config.validation_layers.size());
		instance_info.ppEnabledLayerNames = m_config.validation_layers.data();

		// Attach debug messenger create info. This ONLY tracks messages from the instance
		instance_info.pNext = &m_config.debug_messenger_info;
	}
	else
	{
		instance_info.enabledLayerCount = 0;
		instance_info.pNext = nullptr;
	}

	// Required GLFW Extensions
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	if (glfw_extension_count == 0 || !glfw_extensions)
	{
		AURION_CRITICAL("Could not initialize Vulkan Driver: GLFW is either unavailable or hasn't been initialized.");
		return;
	}

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (m_config.enable_debug_messenger)
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Enable vulkan debug utilities

	// Attach required extensions, if everything is valid
	instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	// Create Vulkan Instance
	if (vkCreateInstance(&instance_info, nullptr, &m_vk_instance) != VK_SUCCESS)
	{
		AURION_CRITICAL("Failed to create Vulkan instance.");
		return;
	}
}

void VulkanDriver::CreateVkDebugMessenger()
{
	if (!m_config.enable_debug_messenger)
		return;

	if (Vulkan_CreateDebugUtilsMessengerEXT(m_vk_instance, &m_config.debug_messenger_info, nullptr, &m_vk_debug_messenger) != VK_SUCCESS)
		AURION_ERROR("[Vulkan Driver] Failed to create debug messenger.");
}

#define VMA_IMPLEMENTATION

#include <macros/AurionLog.h>

#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanDriver::VulkanDriver()
	: m_driver_config(nullptr), m_vk_instance(VK_NULL_HANDLE), m_vk_debug_messenger(VK_NULL_HANDLE)
{
}

VulkanDriver::~VulkanDriver()
{
	m_renderers.clear();

	if (m_vk_debug_messenger != VK_NULL_HANDLE)
		Vulkan_DestroyDebugUtilsMessengerEXT(m_vk_instance, m_vk_debug_messenger, nullptr);

	if (m_vk_instance != VK_NULL_HANDLE)
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

	if (!m_driver_config)
	{
		AURION_WARN("[Vulkan Driver] Cannot initialize driver. Invalid Configuration!");
		return;
	}

	// Use configuration to build vulkan instance
	this->CreateVkInstance();
	this->CreateVkDebugMessenger();
}

IRenderer* VulkanDriver::CreateRenderer()
{
	if (m_vk_instance == VK_NULL_HANDLE)
	{
		AURION_WARN("[Vulkan Driver] Failed to create renderer: VkInstance is NULL!");
		return nullptr;
	}

	if (!m_driver_config)
	{
		AURION_ERROR("[Vulkan Driver] Failed to create renderer: Invalid driver configuration");
		return nullptr;
	}

	if (!m_device_config)
	{
		AURION_ERROR("[Vulkan Driver] Failed to create renderer: Invalid device configuration");
		return nullptr;
	}

	// Create the renderer
	m_renderers.emplace_back(VulkanRenderer());
	VulkanRenderer& renderer = m_renderers.back();
	
	// Initialize a renderer with provided device and driver configurations
	renderer.SetConfiguration(m_vk_instance, m_device_config, m_driver_config->max_frames_in_flight);
	renderer.Initialize();

	// return reference to renderer
	return &renderer;
}

void VulkanDriver::SetConfiguration(VulkanDriverConfiguration& config)
{
	m_driver_config = &config;
}

void VulkanDriver::SetDeviceConfiguration(const VulkanDeviceConfiguration& config)
{
	m_device_config = &config;
}

void VulkanDriver::CreateVkInstance()
{
	// TODO: Enable Validation Layers & Debug Messenger

	// Instance Create info
	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &m_driver_config->app_info;
	instance_info.enabledLayerCount = 0;
	instance_info.ppEnabledLayerNames = nullptr;
	instance_info.pNext = nullptr;

	// Required GLFW Extensions
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (m_driver_config->enable_validation_layers)
	{
		// Query for the number of instance layers
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		// Query for all layer properties
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Ensure all requested layers are available
		for (const char* validationLayer : m_driver_config->validation_layers)
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
					"[Vulkan Driver] One or more requested validation layers are not available!\nFirst is: %s",
					validationLayer
				);
				return;
			}
		}


		// Enable Validation Layers
		// NOTE: For whatever reason... if the following two lines aren't called, draw commands just... fail. FIX ME PLEASE
		instance_info.enabledLayerCount = static_cast<uint32_t>(m_driver_config->validation_layers.size());
		instance_info.ppEnabledLayerNames = m_driver_config->validation_layers.data();
	}

	if (m_driver_config->enable_debug_messenger)
	{
		// Setup Debug Messenger
		m_driver_config->debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		m_driver_config->debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		m_driver_config->debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		m_driver_config->debug_messenger_info.pfnUserCallback = Vulkan_DebugCallback;
		m_driver_config->debug_messenger_info.pUserData = nullptr;

		// Attach debug messenger create info. This ONLY tracks messages from the instance
		instance_info.pNext = &m_driver_config->debug_messenger_info;

		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Attach required extensions
	instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	// Create Vulkan Instance
	if (vkCreateInstance(&instance_info, nullptr, &m_vk_instance) != VK_SUCCESS)
	{
		AURION_CRITICAL("Failed to create Vulkan instance.");
		return;
	}

	AURION_INFO("[Vulkan Driver] Vulkan Instance Created!");
}

void VulkanDriver::CreateVkDebugMessenger()
{
	if (!m_driver_config->enable_debug_messenger)
		return;

	if (Vulkan_CreateDebugUtilsMessengerEXT(m_vk_instance, &m_driver_config->debug_messenger_info, nullptr, &m_vk_debug_messenger) != VK_SUCCESS)
		AURION_ERROR("[Vulkan Driver] Failed to create debug messenger.");
}

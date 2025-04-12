module;

#include <vector>

#include <vulkan/vulkan.h>

export module Vulkan:Driver;

import Graphics;

import :Renderer;
import :Device;
import :Pipeline;

export
{
	struct VulkanDriverConfiguration
	{
		VkApplicationInfo app_info{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
		std::vector<const char*> validation_layers{};
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
		uint32_t max_frames_in_flight = 1;
		bool enable_validation_layers = true;
		bool enable_debug_messenger = true;
	};

	class VulkanDriver : public IGraphicsDriver
	{
	public:
		VulkanDriver();
		virtual ~VulkanDriver() override;

		const uint64_t GetType() override;
		
		virtual void Initialize() override;

		virtual IRenderer* CreateRenderer() override;

		void SetConfiguration(VulkanDriverConfiguration& config);

		void SetDeviceConfiguration(const VulkanDeviceConfiguration& config);

	private:
		void CreateVkInstance();
		void CreateVkDebugMessenger();

	private:
		VulkanDriverConfiguration* m_driver_config;
		const VulkanDeviceConfiguration* m_device_config;
		VkInstance m_vk_instance;

		VkDebugUtilsMessengerEXT m_vk_debug_messenger;
		
		std::vector<VulkanRenderer> m_renderers;
	};
}
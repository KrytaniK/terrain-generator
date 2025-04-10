module;

#include <cstdint>
#include <set>
#include <vector>
#include <unordered_map>

#include <vulkan/vulkan.h>

export module Vulkan:Renderer;

import Graphics;

import :Device;
import :Window;
import :Pipeline;

import :Command;

export
{
	class VulkanRenderer : public IRenderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer() override;

		void Init(const VkInstance& vk_instance, const VulkanDeviceRequirements& logical_device_reqs, const uint32_t& max_in_flight_frames);

		void Shutdown();

		virtual void BeginFrame() override;

		virtual void EndFrame() override;

		virtual bool AddWindow(const Aurion::WindowHandle& handle) override;

		virtual void SetWindowEnabled(const Aurion::WindowHandle& handle, bool enabled) override;

		virtual VulkanWindow* GetGraphicsWindow(const Aurion::WindowHandle& handle) override;
		virtual VulkanWindow* GetGraphicsWindow(const uint64_t& window_id) override;

		virtual bool RemoveGraphicsWindow(const Aurion::WindowHandle& handle) override;
		virtual bool RemoveGraphicsWindow(const uint64_t& window_id) override;

		VulkanPipelineBuilder* GetPipelineBuilder();

		// Binds a render command to the window for repeated calls. CAUTION: These will NOT be cleared each frame.
		void BindCommand(const Aurion::WindowHandle& window_handle, const std::function<void(const VulkanCommand&)>& command);

		// Submits a render command for execution. Gets cleared every frame
		void SubmitCommand(const Aurion::WindowHandle& window_handle, const std::function<void(const VulkanCommand&)>& command);

		// Submits a render command for immediate execution outside of the primary render loop.
		//void SubmitCommandImmediate();
		
	private:
		VulkanDevice m_logical_device;
		VulkanPipelineBuilder m_pipeline_builder;
		std::vector<VulkanPipeline> m_pipelines;
		std::unordered_map<uint64_t, VulkanWindow> m_windows;
		std::set<uint64_t> m_windows_to_remove;
		uint32_t m_max_in_flight_frames;
	};
}
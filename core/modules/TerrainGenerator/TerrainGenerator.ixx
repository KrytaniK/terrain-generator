module;

#include <span>

export module TerrainGenerator;

import Aurion.Application;
import Aurion.GLFW;

import DebugLayers;
import Graphics;
import Vulkan;

export
{
	class TerrainGenerator : public Aurion::IApplication
	{
	public:
		TerrainGenerator();
		virtual ~TerrainGenerator() override;

		void StartAndRun() override;

	private:
		void Load();
		void Start();
		void Run();
		void Unload();

	private:
		Aurion::GLFWDriver m_window_driver;
		Aurion::GLFWInputContext m_input_context;
		Aurion::GLFWInputDevice* m_mouse;
		Aurion::GLFWInputDevice* m_keyboard;
		VulkanDriver m_vulkan_driver;
		VulkanRenderer* m_renderer;
		std::span<VulkanPipeline> m_render_pipelines;
		bool m_should_close;
		DebugGridConfig m_debug_grid_config;
	};
}
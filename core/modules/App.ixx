module;


export module Application;

import Aurion.Application;
import Aurion.GLFW;

import Vulkan;

import Terrain;

export
{
	class Application : public Aurion::IApplication
	{
	public:
		Application();
		virtual ~Application() override;

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
		bool m_should_close;
		TerrainGenerator m_terrain_generator;
	};
}
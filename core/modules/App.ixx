export module Application;

import Aurion.Application;
import Aurion.GLFW;

import Vulkan;

import World;
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
		VulkanDriver m_vulkan_driver;
		VulkanRenderer* m_renderer;
		World m_world;
		TerrainEventDispatcher m_terrain_event_dispatcher;
		bool m_should_close;
	};
}
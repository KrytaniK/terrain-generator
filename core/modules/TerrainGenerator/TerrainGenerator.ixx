export module TerrainGenerator;

import Aurion.Application;
import Aurion.GLFW;

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
		VulkanDriver m_vulkan_driver;
		VulkanRenderer* m_renderer;
		bool m_should_close;
	};
}
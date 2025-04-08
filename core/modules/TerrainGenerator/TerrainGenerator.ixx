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

		void Render(const VulkanCommand& command);

	private:
		Aurion::GLFWDriver m_window_driver;
		VulkanDriver m_vulkan_driver;
		VulkanRenderer* m_renderer;
		VulkanPipelineBuilder::Result m_render_pipelines;
		bool m_should_close;
	};
}
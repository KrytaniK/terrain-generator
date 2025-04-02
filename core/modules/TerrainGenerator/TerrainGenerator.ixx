export module TerrainGenerator;

import Aurion.Application;
import Aurion.GLFW;

export
{
	class TerrainGenerator : public Aurion::IApplication
	{
	public:
		TerrainGenerator();
		virtual ~TerrainGenerator() override;

		// Inherited via IApplication
		void StartAndRun() override;

	private:
		void Load();
		void Start();
		void Run();
		void Unload();

	private:
		Aurion::GLFWDriver m_window_driver;
	};
}
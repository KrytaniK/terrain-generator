#include <macros/AurionLog.h>

#include <GLFW/glfw3.h>

import TerrainGenerator;

TerrainGenerator::TerrainGenerator()
{
	
}

TerrainGenerator::~TerrainGenerator()
{

}

void TerrainGenerator::StartAndRun()
{
	this->Load();
	this->Start();
	this->Run();
	this->Unload();
}

void TerrainGenerator::Load()
{
	// Potentially load config from file
	Aurion::WindowDriverConfig driver_config{};
	driver_config.max_window_count = 10;

	m_window_driver.Initialize(driver_config);

	// Potentially load vulkan driver config from file
	m_vulkan_driver.Initialize();

	m_renderer = m_vulkan_driver.CreateRenderer();
}

void TerrainGenerator::Start()
{
	Aurion::WindowConfig window_config;
	window_config.title = "Terrain Generator";

	// Create the main window
	Aurion::WindowHandle main_window = m_window_driver.InitWindow(window_config);

	m_renderer->AddWindow(main_window);
}

void TerrainGenerator::Run()
{
	m_should_close = false;

	Aurion::IWindow* a = m_window_driver.GetWindow("Terrain Generator").window;
	while (!m_should_close)
	{
		m_renderer->BeginFrame();
		m_renderer->EndFrame();

		m_should_close = !a->IsOpen();
	}
}

void TerrainGenerator::Unload()
{

}
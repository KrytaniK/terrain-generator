#include <macros/AurionLog.h>

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
	Aurion::WindowDriverConfig driver_config{};
	driver_config.max_window_count = 10;

	m_window_driver.Initialize(driver_config);
}

void TerrainGenerator::Start()
{
	Aurion::WindowConfig window_config;
	window_config.title = "New Window";

	m_window_driver.InitWindow(window_config).window;
}

void TerrainGenerator::Run()
{
	Aurion::IWindow* window = m_window_driver.GetWindow("New Window").window;
	while (window->IsOpen())
		window->Update();
}

void TerrainGenerator::Unload()
{

}
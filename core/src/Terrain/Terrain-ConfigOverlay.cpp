#include <macros/AurionLog.h>

#include <imgui.h>

import Terrain;

TerrainConfigOverlay::TerrainConfigOverlay()
	: m_enabled(true)
{

}

TerrainConfigOverlay::~TerrainConfigOverlay()
{

}

void TerrainConfigOverlay::Initialize(TerrainGenerator& generator)
{
	m_generator = &generator;
	m_config = &m_generator->GetConfiguration();
}

void TerrainConfigOverlay::Record(const IGraphicsCommand* command)
{
	if (!m_config)
	{
		AURION_ERROR("Failed to render Config UI: Invalid Terrain Generator Config!");
		return;
	}

	ImGui::Begin("Terrain Configuration", &m_enabled, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::SliderFloat("Scale", &m_config->scale, 0.1f, 10.0f))
			m_generator->GenerateTerrain();

		if (ImGui::SliderInt("Chunk Size", &m_config->chunk_size, 1, 32))
			m_generator->GenerateTerrain();

		if (ImGui::Checkbox("Wireframe", &m_config->wireframe))
			m_generator->GenerateTerrain();

	ImGui::End();
}

void TerrainConfigOverlay::Enable()
{
	m_enabled = true;
}

void TerrainConfigOverlay::Disable()
{
	m_enabled = false;
}

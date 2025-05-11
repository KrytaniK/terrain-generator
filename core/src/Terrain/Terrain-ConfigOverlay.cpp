#include <macros/AurionLog.h>

#include <imgui.h>

#include <glm/glm.hpp>

import Terrain;

TerrainConfigOverlay::TerrainConfigOverlay()
	: m_enabled(true)
{

}

TerrainConfigOverlay::~TerrainConfigOverlay()
{

}

void TerrainConfigOverlay::Initialize(TerrainEventDispatcher& event_dispatcher)
{
	// Attach event dispatcher
	m_event_dispatcher = &event_dispatcher;

	// Attach configuration structure
	m_update_event.new_config = &m_config;
	m_update_event.type = 0;
}

void TerrainConfigOverlay::Record(const IGraphicsCommand* command)
{
	if (!m_enabled)
		return;

	ImGui::Begin("Terrain Configuration", &m_enabled, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::InputInt("Chunk Resolution", &m_config.chunk_resolution, 1, 100))
			m_event_dispatcher->Dispatch(&m_update_event);

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

#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>

#include <imgui.h>

import DebugOverlays;
import DebugLayers;

DebugGridOverlay::DebugGridOverlay()
	: m_enabled(true)
{

}

DebugGridOverlay::~DebugGridOverlay()
{

}

void DebugGridOverlay::Initialize(DebugGridConfig* config)
{
	m_config = config;
}

void DebugGridOverlay::Record(const IGraphicsCommand* command)
{
	ImGui::Begin("Debug Grid Configuration", &m_enabled);
		ImGui::Text("Line Width");
		ImGui::SameLine();
		ImGui::SliderFloat("Line Width", &m_config->line_width, 1.f, 10.f);

		ImGui::Text("Cell Size");
		ImGui::SameLine();
		ImGui::SliderFloat("Cell Size", &m_config->cell_size, 0.1f, 10.f, "%.3f", ImGuiSliderFlags_ClampOnInput);

		ImGui::Text("Anti-Aliasing Scale");
		ImGui::SameLine();
		ImGui::SliderFloat("Anti-Aliasing Scale", &m_config->anti_aliasing, 1.f, 10.f, "%.3f", ImGuiSliderFlags_ClampOnInput);
	ImGui::End();
}

void DebugGridOverlay::Enable()
{
	m_enabled = true;
}

void DebugGridOverlay::Disable()
{
	m_enabled = false;
}

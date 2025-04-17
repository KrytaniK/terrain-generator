#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>
#include <imgui.h>

import ImGuiDemo;
import Vulkan;

ImGuiDemoOverlay::ImGuiDemoOverlay()
{

}

ImGuiDemoOverlay::~ImGuiDemoOverlay()
{

}

void ImGuiDemoOverlay::Record(const IGraphicsCommand* command)
{
	if (!m_enabled)
		return;

	ImGui::ShowDemoWindow();
}

void ImGuiDemoOverlay::Enable()
{
	m_enabled = true;
}

void ImGuiDemoOverlay::Disable()
{
	m_enabled = false;
}

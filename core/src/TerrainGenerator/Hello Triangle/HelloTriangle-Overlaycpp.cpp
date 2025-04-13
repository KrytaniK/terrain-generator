#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>
#include <imgui.h>

import HelloTriangle;
import Vulkan;

HelloTriangleOverlay::HelloTriangleOverlay()
{

}

HelloTriangleOverlay::~HelloTriangleOverlay()
{

}

void HelloTriangleOverlay::Record(const IGraphicsCommand* command)
{
	if (!m_enabled)
		return;

	ImGui::ShowDemoWindow();
}

void HelloTriangleOverlay::Enable()
{
	m_enabled = true;
}

void HelloTriangleOverlay::Disable()
{
	m_enabled = false;
}

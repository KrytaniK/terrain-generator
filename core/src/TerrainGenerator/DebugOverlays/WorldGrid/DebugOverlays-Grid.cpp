#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>

#include <imgui.h>

import DebugOverlays;
import DebugLayers;

DebugGridOverlay::DebugGridOverlay()
	: m_enabled(false)
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
	return;
}

void DebugGridOverlay::Enable()
{
	m_enabled = true;
}

void DebugGridOverlay::Disable()
{
	m_enabled = false;
}

#include <macros/AurionLog.h>

import Terrain;

TerrainEventListener::TerrainEventListener()
	: m_event_type(0)
{

}

TerrainEventListener::TerrainEventListener(const uint64_t& event_type)
	: m_event_type(event_type)
{

}

TerrainEventListener::~TerrainEventListener()
{

}

uint64_t TerrainEventListener::GetEventType()
{
	return m_event_type;
}

void TerrainEventListener::OnEvent(Aurion::IEvent* event)
{
	if (event->type != m_event_type)
	{
		AURION_ERROR("[TerrainEventListener::OnEvent] Attempted to execute an event callback with mismatched event types! Event Type: [%d], Listener Type: [%d]", event->type, m_event_type);
		return;
	}

	if (m_callback)
		m_callback(static_cast<TerrainUpdateEvent*>(event));
}

void TerrainEventListener::Bind(const std::function<void(TerrainUpdateEvent*)>& callback)
{
	m_callback = callback;
}

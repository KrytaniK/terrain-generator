#include <macros/AurionLog.h>

#include <vector>

import Terrain;

TerrainEventDispatcher::TerrainEventDispatcher()
	: m_event_type(0)
{

}

TerrainEventDispatcher::TerrainEventDispatcher(const uint64_t& event_type)
	: m_event_type(event_type)
{

}

TerrainEventDispatcher::~TerrainEventDispatcher()
{

}

uint64_t TerrainEventDispatcher::GetEventType()
{
	return m_event_type;
}

bool TerrainEventDispatcher::AddEventListener(Aurion::IEventListener* listener)
{
	// Ensure matching types
	if (listener->GetEventType() != m_event_type)
		return false;

	m_listeners.push_back(static_cast<TerrainEventListener*>(listener));

	return true;
}

bool TerrainEventDispatcher::RemoveEventListener(Aurion::IEventListener* listener)
{
	// Ensure matching types
	if (listener->GetEventType() != m_event_type)
		return false;

	// Linear search for listener
	int index_to_remove = -1;
	for (size_t i = 0; i < m_listeners.size(); i++)
	{
		if (m_listeners[i] == listener)
		{
			index_to_remove = (int)i;
		}
	}

	// Bail if it doesn't exist
	if (index_to_remove < 0)
		return false;

	// Otherwise, remove it.
	m_listeners.erase(m_listeners.begin() + (size_t)(index_to_remove));

	return true;
}

void TerrainEventDispatcher::Dispatch(Aurion::IEvent* event)
{
	for (size_t i = 0; i < m_listeners.size(); i++)
		m_listeners[i]->OnEvent(event);
}

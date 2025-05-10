module;

#include <vector>
#include <functional>

export module Terrain:Events;

import :Data;

import Aurion.Events;

export
{
	struct TerrainEvent : public Aurion::IEvent
	{
		TerrainConfig* new_config;
	};

	class TerrainEventListener : public Aurion::IEventListener
	{
	public:
		TerrainEventListener();
		TerrainEventListener(const uint64_t& event_type);
		virtual ~TerrainEventListener() override;

		virtual uint64_t GetEventType() override;

		virtual void OnEvent(Aurion::IEvent* event) override;

		void Bind(const std::function<void()>& callback);
		void Bind(const std::function<void(TerrainEvent*)>& callback);

	private:
		uint64_t m_event_type;
		std::function<void()> m_void_callback;
		std::function<void(TerrainEvent*)> m_callback;
	};

	class TerrainEventDispatcher : public Aurion::IEventDispatcher
	{
	public:
		TerrainEventDispatcher();
		TerrainEventDispatcher(const uint64_t& event_type);
		virtual ~TerrainEventDispatcher() override;

		virtual uint64_t GetEventType() override;

		virtual bool AddEventListener(Aurion::IEventListener* listener) override;

		virtual bool RemoveEventListener(Aurion::IEventListener* listener) override;

		virtual void Dispatch(Aurion::IEvent* event = nullptr) override;

	private:
		uint64_t m_event_type;
		std::vector<TerrainEventListener*> m_listeners;
	};
}
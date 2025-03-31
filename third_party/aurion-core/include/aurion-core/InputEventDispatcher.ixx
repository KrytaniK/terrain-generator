module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:EventDispatcher;

import Aurion.Events;

import :EventListener;

export namespace Aurion
{
	class AURION_API InputEventDispatcher : public IEventDispatcher
	{
	public:
		InputEventDispatcher();
		InputEventDispatcher(const uint64_t& event_type);
		virtual ~InputEventDispatcher() override;

		uint64_t GetEventType() override;

		bool AddEventListener(IEventListener* listener) override;

		bool RemoveEventListener(IEventListener* listener) override;

		void Dispatch(IEvent* event) override;

	private:
		uint64_t m_event_type;
		size_t m_listener_count;
		size_t m_capacity;
		InputEventListener** m_listeners;
	};
}
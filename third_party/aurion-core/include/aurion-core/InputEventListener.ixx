module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:EventListener;

import Aurion.Events;

import :Event;

export namespace Aurion
{
	typedef AURION_API void(*InputVoidCallback)();
	typedef AURION_API void(*InputEventCallback)(InputEvent*);

	class AURION_API InputEventListener : public IEventListener
	{
	public:
		InputEventListener();
		InputEventListener(const uint64_t& event_type);
		virtual ~InputEventListener() override;

		uint64_t GetEventType() override;

		virtual void OnEvent(IEvent* event) override;

		bool Bind(const InputVoidCallback& callback);
		bool Bind(const InputEventCallback& callback);

	private:
		uint64_t m_event_type;
		InputVoidCallback m_base_callback;
		InputEventCallback m_event_callback;
	};
}
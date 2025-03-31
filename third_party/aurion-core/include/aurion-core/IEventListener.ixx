module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Events:Listener;

import :Event;

export namespace Aurion
{
	class AURION_API IEventListener
	{
	public:
		virtual ~IEventListener() = default;

		virtual uint64_t GetEventType() = 0;

		virtual void OnEvent(IEvent* event) = 0;
	};
}
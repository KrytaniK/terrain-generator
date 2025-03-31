module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Events:Dispatcher;

import Aurion.Memory;

import :Event;
import :Listener;

export namespace Aurion
{
	class AURION_API IEventDispatcher
	{
	public:
		virtual ~IEventDispatcher() = default;

		virtual uint64_t GetEventType() = 0;

		virtual bool AddEventListener(IEventListener* listener) = 0;

		virtual bool RemoveEventListener(IEventListener* listener) = 0;

		virtual void Dispatch(IEvent* event) = 0;
	};
}
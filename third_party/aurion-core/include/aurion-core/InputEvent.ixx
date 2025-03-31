module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:Event;

import Aurion.Events;

export namespace Aurion
{
	struct AURION_API InputEvent : IEvent
	{
		uint64_t input_code;
		uint64_t value_size;
		void* value;
	};
}
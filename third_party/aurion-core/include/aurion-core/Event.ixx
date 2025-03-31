module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Events:Event;

export namespace Aurion
{
	struct AURION_API IEvent
	{
		uint64_t type = UINT64_MAX;
		bool propagates = true;
	};
}
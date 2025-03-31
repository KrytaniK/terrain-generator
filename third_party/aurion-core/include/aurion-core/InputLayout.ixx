module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:Layout;

import :Control;

export namespace Aurion
{
	// An input layout represents the memory layout of a device's controls (such as the buttons on a keyboard, or the scroll direction of a mouse)
	struct AURION_API InputDeviceLayout
	{
		const char* name = nullptr;
		uint32_t id = 0; // ID for this layout
		uint16_t size = 0; // Total size (in bytes) of this layout in memory
		uint16_t control_count = 0; // The number of controls this layout uses
		InputControlInfo* control_infos = nullptr; // List of control information structurues 
		InputControlLayout* control_layouts = nullptr; // List of device control memory layouts 
	};
}
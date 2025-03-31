module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:Device;

import :Layout;
import :Control;

export namespace Aurion
{
	struct AURION_API InputDeviceInfo
	{
		uint64_t id = 0;
		uint64_t classification = 0; // Classification Identifier, as an unsigned 64-bit integer (such as gamepad, keyboard, etc.)
		const char* alias = nullptr;
		const char* product_name = nullptr; // Name of the product
		const char* product_manufacturer = nullptr; // Name of the manufacturer
		const char* product_version = nullptr; // Product version, as a string
		const char* product_interface = nullptr; // Name of the interface making the device available (such as HID).
		const char* product_ext = nullptr; // Extension string for interface-specific device capabilities (such as HID information)
	};
	
	class AURION_API IInputDevice
	{
	public:
		virtual ~IInputDevice() = default;

		// Retrieve basic device information
		virtual const InputDeviceInfo& GetInfo() = 0;

		virtual const uint32_t& GetLayoutID() = 0;

		// Searches for the control, provided the input_code for that control
		virtual IInputControl* GetControl(const uint64_t& input_code) = 0;

		// Maps existing controls to device memory
		virtual bool Map(const InputDeviceLayout& layout) = 0;
	};
}
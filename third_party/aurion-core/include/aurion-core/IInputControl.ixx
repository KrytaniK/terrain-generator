module;

#include <macros/AurionExport.h>

#include <cstdint>
#include <string>

export module Aurion.Input:Control;

export namespace Aurion
{
	// The general info for a control
	struct AURION_API InputControlInfo
	{
		const char* name = nullptr;
		uint64_t input_code = 0;
		uint64_t type = 0;
	};

	// The memory layout for a control
	struct AURION_API InputControlLayout
	{
		uint16_t byte_offset = 0;
		uint16_t size_in_bytes = 0;
		uint8_t bit_offset = 0;
		uint8_t size_in_bits = 0;
		uint8_t child_count = 0;
	};

	class AURION_API IInputControl
	{
	public:
		virtual ~IInputControl() = default;

		virtual const InputControlInfo& GetInfo() = 0;

		virtual const InputControlLayout& GetLayout() = 0;

		virtual const uint64_t& GetInputCode() = 0;

		virtual const uint64_t& GetControlType() = 0;

		virtual void Initialize(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout) = 0;

		virtual bool Update(void* value, const uint16_t& value_size) = 0;
	};
}
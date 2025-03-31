module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Input:Context;

import :Device;
import :Layout;

export namespace Aurion
{
	class AURION_API IInputContext
	{
	public:
		virtual ~IInputContext() = default;

		// Device Management

		virtual IInputDevice* CreateDevice(const InputDeviceInfo& info, const uint32_t& layout_id) = 0;

		virtual IInputDevice* CreateDevice(const InputDeviceInfo& info, const char* layout_name) = 0;

		virtual IInputDevice* CreateDevice(const InputDeviceInfo& info, const InputDeviceLayout& layout) = 0;

		virtual IInputDevice* GetDevice(const uint64_t& id) = 0;

		virtual IInputDevice* GetDevice(const char* name) = 0;

		virtual bool RemoveDevice(const uint64_t& id) = 0;

		virtual bool RemoveDevice(const char* name) = 0;

		virtual bool RemoveDevice(IInputDevice* device) = 0;

		// Layout Management

		virtual InputDeviceLayout* GetLayout(const uint32_t& layout_id) = 0;

		virtual InputDeviceLayout* GetLayout(const char* name) = 0;

		virtual void AddDeviceLayout(const InputDeviceLayout& layout) = 0;

		virtual bool RemoveDeviceLayout(const uint32_t& id) = 0;

		virtual bool RemoveDeviceLayout(const char* name) = 0;

		virtual bool RemoveDeviceLayout(const InputDeviceLayout& layout) = 0;
	};
	
}
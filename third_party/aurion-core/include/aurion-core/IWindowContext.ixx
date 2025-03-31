module;

#include <macros/AurionExport.h>

export module Aurion.Window:Context;

import Aurion.Plugin;
import :Driver;

export namespace Aurion
{
	class AURION_API WindowContext : public IPluginContext
	{
	public:
		WindowContext();
		virtual ~WindowContext() override;

		void SetDriver(IWindowDriver* driver);

		IWindowDriver* GetDriver();

	private:
		IWindowDriver* m_driver;
	};
}
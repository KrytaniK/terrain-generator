module;

#include <macros/AurionExport.h>

export module Aurion.Plugin:Loader;

import :Plugin;

export namespace Aurion
{
	class AURION_API IPluginLoader
	{
	public:
		virtual ~IPluginLoader() = default;

		virtual IPlugin* LoadPlugin(const char* source) = 0;
		virtual void UnloadPlugin(IPlugin* plugin) = 0;
	};

	extern "C" AURION_API IPluginLoader* CreatePluginLoader(const size_t& plugin_count);
}
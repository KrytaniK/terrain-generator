module;

#include <macros/AurionExport.h>

export module Aurion.Plugin:Plugin;

import <memory>;
import <cstdint>;

import :Context;

export namespace Aurion
{
	class AURION_API IPlugin
	{
	public:
		virtual ~IPlugin() = default;

		virtual void Initialize(IPluginContext* context) = 0;
	};

	typedef IPlugin* (*FnCreatePluginAlias)();
	typedef void (*FnDestroyPluginAlias)(const IPlugin*);
}
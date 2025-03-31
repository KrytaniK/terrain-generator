module;

#include <macros/AurionExport.h>

export module Aurion.Application;

import <memory>;

export namespace Aurion
{
	class AURION_API IApplication
	{
	public:
		virtual ~IApplication() = default;

		virtual void StartAndRun() = 0;
	};
}
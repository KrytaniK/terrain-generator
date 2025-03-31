module;

#include <macros/AurionExport.h>

export module Aurion.Log:ConsoleLogger;

import :Interface;

export namespace Aurion
{
	class AURION_API ConsoleLogger : public ILogger
	{
	public:
		static ConsoleLogger* Get();

	public:
		ConsoleLogger();
		virtual ~ConsoleLogger() override;

		virtual void Log(const LogLevel& verbosity, const char* format, ...) override;
	};
}
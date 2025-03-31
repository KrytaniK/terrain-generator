module;

#include <macros/AurionExport.h>

export module Aurion.Log:FileLogger;

import Aurion.FileSystem;

import :Interface;

export namespace Aurion
{
	class AURION_API FileLogger : public ILogger
	{
	private:
		inline static IFileSystem* s_file_system = nullptr;

	public:
		static FileLogger* Get();

	public:
		FileLogger();
		FileLogger(IFileSystem* fs, const char* out_file_path);
		virtual ~FileLogger() override;

		virtual void Log(const LogLevel& verbosity, const char* format, ...) override;

	private:
		FSFileHandle m_log_file;
	};
}
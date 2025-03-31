module;

#include <macros/AurionExport.h>

export module Aurion.Log:Interface;

export namespace Aurion
{
	constexpr const char* c_log_prefix_trace = "[TRACE] ";
	constexpr const char* c_log_prefix_info = "[INFO] ";
	constexpr const char* c_log_prefix_warn = "[WARN] ";
	constexpr const char* c_log_prefix_error = "[ERROR] ";
	constexpr const char* c_log_prefix_critical = "[CRITICAL] ";

	constexpr const char* c_text_bg_white = "\x1B[48;5;255m";
	constexpr const char* c_text_bg_red = "\x1B[48;5;88m";
	constexpr const char* c_text_bg_green = "\x1B[48;5;82m";
	constexpr const char* c_text_bg_blue = "\x1B[48;5;220m";
	constexpr const char* c_text_bg_cyan = "\x1B[48;5;123m";

	constexpr const char* c_text_color_white = "\x1B[38;5;255m";
	constexpr const char* c_text_color_red = "\x1B[38;5;196m";
	constexpr const char* c_text_color_green = "\x1B[38;5;82m";
	constexpr const char* c_text_color_blue = "\x1B[38;5;21m";
	constexpr const char* c_text_color_yellow = "\x1B[38;5;220m";
	constexpr const char* c_text_color_cyan = "\x1B[38;5;123m";

	constexpr const char* c_text_color_end = "\x1B[0m";

	typedef AURION_API enum LogLevel
	{
		LL_TRACE = 0x00,
		LL_INFO = 0x01,
		LL_WARN = 0x02,
		LL_ERROR = 0x04,
		LL_CRITICAL = 0x08,
	} LogLevel;

	class AURION_API ILogger
	{
	public:
		virtual ~ILogger() = default;

		virtual void Log(const LogLevel& verbosity, const char* format, ...) = 0;
	};
}
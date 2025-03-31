#pragma once

import Aurion.Log;

#define AURION_TRACE(...)		Aurion::ConsoleLogger::Get()->Log(Aurion::LL_TRACE, __VA_ARGS__)
#define AURION_INFO(...)		Aurion::ConsoleLogger::Get()->Log(Aurion::LL_INFO, __VA_ARGS__)
#define AURION_WARN(...)		Aurion::ConsoleLogger::Get()->Log(Aurion::LL_WARN, __VA_ARGS__)
#define AURION_ERROR(...)		Aurion::ConsoleLogger::Get()->Log(Aurion::LL_ERROR, __VA_ARGS__)
#define AURION_CRITICAL(...)	Aurion::ConsoleLogger::Get()->Log(Aurion::LL_CRITICAL, __VA_ARGS__)

#define AURION_FTRACE(...)		Aurion::FileLogger::Get()->Log(Aurion::LL_TRACE, __VA_ARGS__)
#define AURION_FINFO(...)		Aurion::FileLogger::Get()->Log(Aurion::LL_INFO, __VA_ARGS__)
#define AURION_FWARN(...)		Aurion::FileLogger::Get()->Log(Aurion::LL_WARN, __VA_ARGS__)
#define AURION_FERROR(...)		Aurion::FileLogger::Get()->Log(Aurion::LL_ERROR, __VA_ARGS__)
#define AURION_FCRITICAL(...)	Aurion::FileLogger::Get()->Log(Aurion::LL_CRITICAL, __VA_ARGS__)
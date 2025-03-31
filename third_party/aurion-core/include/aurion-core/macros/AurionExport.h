#pragma once

#ifdef AURION_PLATFORM_WINDOWS
	#define AURION_DLL_EXPORT __declspec(dllexport)
	#define AURION_DLL_IMPORT __declspec(dllimport)
#else
	#define AURION_DLL_EXPORT
	#define AURION_DLL_IMPORT
#endif

#ifdef AURION_DLL
	#define AURION_API AURION_DLL_EXPORT
#else
	#define AURION_API AURION_DLL_IMPORT
#endif
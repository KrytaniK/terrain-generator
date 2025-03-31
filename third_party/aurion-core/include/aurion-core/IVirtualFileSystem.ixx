module;

#include <macros/AurionExport.h>

export module Aurion.VFS:Interface;

export namespace Aurion
{
	class AURION_API IVirtualFileSystem
	{
	public:
		virtual ~IVirtualFileSystem() = default;

		virtual void Mount(const char* virtual_dir, const char* system_dir) = 0;

		virtual void Dismount(const char* virtual_dir, const char* system_dir) = 0;

		virtual const char* Resolve(const char* virtual_path) = 0;

		virtual bool DirectoryExists(const char* virtual_dir) = 0;

		virtual void* ReadFile(const char* virtual_path) = 0;

		virtual bool WriteFile(const char* virtual_path, void* buffer) = 0;
	};
}
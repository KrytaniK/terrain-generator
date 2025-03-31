module;

#include <macros/AurionExport.h>
#include <cstdint>

export module Aurion.FileSystem:DirectoryHandle;

import Aurion.Memory;

import :FileHandle;

export namespace Aurion
{
	class IFileSystem;

	struct AURION_API FSDirectoryInfo
	{
		char* name = nullptr;
		size_t file_count = 0;
		size_t dir_count = 0;
	};

	class AURION_API FSDirectoryHandle
	{
	public:
		FSDirectoryHandle();
		FSDirectoryHandle(IFileSystem* fs, const char* path, const bool& force_create);
		~FSDirectoryHandle();

		void Register(IFileSystem* fs, const char* path, const bool& force_create = false);

		const FSDirectoryInfo& GetInfo();

		FSFileHandle* FindFile(const char* file_name, const bool& recursive = false);

		FSDirectoryHandle* FindDirectory(const char* dir_name, const bool& recursive = false);

		FSFileCollection FindAll(const char* extension, const bool& recursive = false);

		FSFileHandle CreateFile(const char* relative_path);

		FSDirectoryHandle* CreateDirectory(const char* relative_path);

	private:
		void FindAll_Recursive(const char* extension, FSFileCollection& out_collection, size_t& created_count);

	private:
		uint64_t m_system_handle;
		IFileSystem* m_file_system;
		FSDirectoryInfo m_info;
		FSFileHandle* m_files;
		FSDirectoryHandle* m_subdirs;
	};
}
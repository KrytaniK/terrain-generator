module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.FileSystem:Interface;

import :DirectoryHandle;
import :FileHandle;
import :FileData;

export namespace Aurion
{
	class AURION_API IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual uint64_t GenerateHandle(const char* path, const bool& force_create) = 0;

		virtual FSFileHandle OpenFile(const char* path, const bool& force_create) = 0;

		virtual bool GetAllFiles(const char* path, FSFileHandle*& out_files, size_t& out_count) = 0;

		virtual bool GetAllDirectories(const char* path, FSDirectoryHandle*& out_dirs, size_t& out_count) = 0;

		virtual bool CloseFile(const uint64_t& handle) = 0;

		virtual void GetFileInfo(const char* path, FSFileInfo& out_info, const bool& force_close = true) = 0;
		virtual void GetFileInfo(const char* path, FSFileInfo& out_info, const uint64_t& handle, const bool& force_close = true) = 0;

		virtual void Read(const char* path, FSFileData* out_data) = 0;
		virtual void Read(const uint64_t& handle, FSFileData* out_data) = 0;

		virtual bool Write(const char* path, void* buffer, const size_t& size, const size_t& offset) = 0;
		virtual bool Write(const uint64_t& handle, void* buffer, const size_t& size, const size_t& offset) = 0;

		virtual bool DirectoryExists(const char* path) = 0;

		virtual bool FileExists(const char* path) = 0;

	private:

		virtual bool IsFilePath(const char* path) = 0;

		virtual bool IsDirPath(const char* path) = 0;
	};
}
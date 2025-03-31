module;

#include <macros/AurionExport.h>
#include <cstdint>

export module Aurion.FileSystem:FileHandle;

import :FileData;

export namespace Aurion
{
	class IFileSystem; // Forward declare file system interface

	struct AURION_API FSFileInfo
	{
		uint64_t creation_time = 0;
		uint64_t last_modified_time = 0;
		uint64_t last_accessed_time = 0;
		char* name = nullptr;
		char* extension = nullptr;
	};

	class AURION_API FSFileHandle
	{
	public:
		FSFileHandle();
		FSFileHandle(IFileSystem* fs, const char* path, const bool& force_create = false); // Path constructor
		~FSFileHandle();

		// Move constructors
		FSFileHandle(FSFileHandle&& other);
		FSFileHandle& operator=(FSFileHandle&& other);

		// Remove copy constructors
		FSFileHandle(FSFileHandle& other);
		FSFileHandle& operator=(FSFileHandle& other);

		void Register(IFileSystem* fs, const char* path, const bool& force_create = false);

		void* Read();

		bool Write(void* buffer, const size_t& size, const size_t& offset);

		const size_t& GetSize();

		const FSFileInfo& GetInfo();

	private:
		IFileSystem* m_file_system;
		const char* m_path;
		FSFileData* m_file_data;
		uint64_t m_system_handle;
		size_t m_reference_count;
		FSFileInfo m_info;
	};

	struct AURION_API FSFileCollection
	{
		FSFileHandle* handles = nullptr;
		size_t count = 0;
	};
}
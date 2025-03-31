module;

#include <macros/AurionExport.h>
#include <cstdint>
#include <cstdlib>
#include <utility>

export module Aurion.FileSystem:FileData;

export namespace Aurion
{
	class AURION_API FSFileData
	{
	public:
		FSFileData() : m_data(nullptr), m_size(0) {};

		FSFileData(const size_t& size) : m_data(nullptr), m_size(size) {
			this->Allocate(size);
		};

		~FSFileData()
		{
			free(m_data);
			m_data = nullptr;
		};

		bool Allocate(const size_t& size)
		{
			// Can't allocate if the size is not 0 or the data has already been allocated
			if (m_size != 0 || m_data)
				return false;

			m_size = size;

			m_data = calloc(1, m_size);

			return m_data != nullptr;
		}

		void* Get() { return m_data; };

		const size_t& Size() { return m_size; };

		// Move constructors
		FSFileData(FSFileData&& other)
		{ 
			m_data = std::move(other.m_data); 
			other.m_data = nullptr;
		};

		FSFileData& operator=(FSFileData&& other)
		{
			m_data = std::move(other.m_data);
			other.m_data = nullptr;
			return *this;
		};

		// Remove copy constructors
		FSFileData(FSFileData&) = delete;
		FSFileData(const FSFileData&) = delete;
		FSFileData& operator=(FSFileData&) = delete;
		FSFileData& operator=(const FSFileData&) = delete;

	private:
		void* m_data;
		size_t m_size;
	};
}
module;

#include <macros/AurionExport.h>

export module Aurion.Memory:LinearAllocator;

import :Allocator;

export namespace Aurion
{
	class AURION_API LinearAllocator : public IMemoryAllocator
	{
	public:
		LinearAllocator(); // Always default constructable
		virtual ~LinearAllocator() override;

		// Still allow move operations
		LinearAllocator(LinearAllocator&& other);
		LinearAllocator& operator=(LinearAllocator&& other);

		virtual void Initialize(const size_t& chunk_size, const size_t& chunk_count = 1) override;

		virtual void* Allocate(const size_t& size, const size_t& alignment = 16) override;

		virtual void Free(void* ptr = nullptr) override; // Empty

		virtual void Reset() override; // Reset state

		virtual bool IsMapped(void* ptr) override;

	private:
		void* m_start;
		size_t m_offset;
		size_t m_max_offset;
	};
}
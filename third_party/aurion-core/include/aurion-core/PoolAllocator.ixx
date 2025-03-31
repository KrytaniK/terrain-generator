module;

#include <macros/AurionExport.h>

export module Aurion.Memory:PoolAllocator;

import :Allocator;

export namespace Aurion
{
	class AURION_API PoolAllocator : public IMemoryAllocator
	{
	public:
		PoolAllocator(); // Always default constructable
		virtual ~PoolAllocator() override;

		// Still allow move operations
		PoolAllocator(PoolAllocator&& other);
		PoolAllocator& operator=(PoolAllocator&& other);

		virtual void Initialize(const size_t& chunk_size, const size_t& chunk_count = 1) override;

		virtual void* Allocate(const size_t& size, const size_t& alignment = 16) override;

		void* Allocate();

		virtual void Free(void* ptr = nullptr) override;

		virtual void Reset() override; // Reset state

		virtual bool IsMapped(void* ptr) override;

	private:
		void* m_start;
		void** m_free_list;
		size_t m_chunk_count;
		size_t m_chunk_size;
	};
}
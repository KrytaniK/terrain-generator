module;

#include <macros/AurionExport.h>
#include <cstdint>

export module Aurion.Memory:Allocator;

export namespace Aurion
{
	struct AURION_API AllocationHeader
	{
		uint8_t padding = 0;
	};

	class AURION_API IMemoryAllocator
	{
	public:
		IMemoryAllocator() = default;
		virtual ~IMemoryAllocator() = default;

		// Still allow move operations
		IMemoryAllocator(IMemoryAllocator&&) = default;
		IMemoryAllocator& operator=(IMemoryAllocator&&) = default;

		virtual void Initialize(const size_t& chunk_size, const size_t& chunk_count = 1) = 0;

		virtual void* Allocate(const size_t& size, const size_t& alignment = 16) = 0;

		virtual void Free(void* ptr = nullptr) = 0;

		virtual void Reset() = 0;

		virtual bool IsMapped(void* ptr) = 0;

		// No copies or explicit assignment operations
		IMemoryAllocator(IMemoryAllocator&) = delete;
		IMemoryAllocator(const IMemoryAllocator&) = delete;
		IMemoryAllocator& operator=(IMemoryAllocator&) = delete;
		IMemoryAllocator& operator=(const IMemoryAllocator&) = delete;
	};
}
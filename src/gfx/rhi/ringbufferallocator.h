#pragma once
#include "resourcepool.h"

namespace limbo::RHI
{
	class Buffer;
	class CommandQueue;
	class CommandContext;

	struct PreDeletedAllocation
	{
		uint64 FenceValue;
		uint64 Offset;
		uint64 Size;
	};

	/**
	 * Allocation made from the ring buffer allocator. This memory will be freed when the scope ends.
	 */
	struct RingBufferAllocation
	{
		CommandContext*		Context;
		Buffer*				Buffer;
		uint64				Offset;
		uint64				Size;
		void*				MappedData;
		uint64				GPUAddress;
	};

	/**
	 * This allocator allocates a big block of memory that is both CPU and GPU visible,
	 * and then makes small allocations inside that block of memory.
	 *
	 * It's used to allocate temporary upload buffers. Buffers that are only used to
	 * upload texture data or to upload data to a buffer that is allocated on the GPU.
	 */
	class RingBufferAllocator
	{
		using PreDeletedQueue = std::queue<PreDeletedAllocation>;

	private:
		Handle<Buffer>	m_Buffer;
		uint64			m_TotalSize;
		uint64			m_CurrentOffset;
		void*			m_MappedData;

		std::string		m_Name;

		CommandQueue*	m_Queue;
		PreDeletedQueue	m_PreDeletedList;
		
	public:
		RingBufferAllocator(uint64 size, const char* name);
		~RingBufferAllocator();

		void Allocate(uint64 size, RingBufferAllocation& allocation);
		void AllocateTemp(uint64 size, RingBufferAllocation& allocation);
		void Free(RingBufferAllocation& allocation);
	};
}

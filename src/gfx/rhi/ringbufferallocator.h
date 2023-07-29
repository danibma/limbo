#pragma once

#include "definitions.h"
#include "resourcepool.h"

namespace limbo::gfx
{
	struct RingBufferAllocation
	{
		Handle<class Buffer>	buffer;
		uint64					offset;
		void*					mappedData;
	};

	class RingBufferAllocator
	{
	public:
		 static RingBufferAllocator* ptr;

	public:
		RingBufferAllocator(uint64 size);
		~RingBufferAllocator();

		bool allocate(uint64 size, RingBufferAllocation& allocation);

	private:
		Handle<class Buffer>	m_buffer;
		void*					m_mappedData;
		uint64					m_maxSize;
		uint64					m_currentOffset;
	};
}

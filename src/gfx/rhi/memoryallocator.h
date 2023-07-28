#pragma once

#include "definitions.h"

namespace limbo::gfx
{
#define MEMORY_ALLOCATOR_MAX_UPLOAD 1024

	class MemoryAllocator
	{
	private:
		struct UploadBuffersList
		{
			ID3D12Resource* resources[MEMORY_ALLOCATOR_MAX_UPLOAD];
			uint32			nextAvailableIndex = 0;
		};

		UploadBuffersList m_uploadBuffersPerFrame[NUM_BACK_BUFFERS];

	public:
		 static MemoryAllocator* ptr;

	public:
		MemoryAllocator() = default;
		~MemoryAllocator();

		ID3D12Resource* createUploadBuffer(uint32 size);
		void flushUploadBuffers(uint32 frameIndex);
	};
}

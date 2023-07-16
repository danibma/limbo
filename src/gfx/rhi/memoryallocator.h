#pragma once

#include "definitions.h"

#include <vector>

namespace limbo::gfx
{
	class MemoryAllocator
	{
	private:
		std::vector<ID3D12Resource*> m_uploadBuffers;

	public:
		 static MemoryAllocator* ptr;

	public:
		MemoryAllocator() = default;
		~MemoryAllocator();

		ID3D12Resource* createUploadBuffer();
		void flushUploadBuffers();
	};
}

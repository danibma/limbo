#pragma once

#include "d3d12definitions.h"

#include <vector>

namespace limbo::rhi
{
	class D3D12MemoryAllocator
	{
	private:
		std::vector<ID3D12Resource*> m_uploadBuffers;

	public:
		 static D3D12MemoryAllocator* ptr;

	public:
		D3D12MemoryAllocator() = default;
		~D3D12MemoryAllocator();

		ID3D12Resource* createUploadBuffer();
		void flushUploadBuffers();
	};
}

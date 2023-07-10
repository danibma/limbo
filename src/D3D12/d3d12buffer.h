#pragma once

#include "buffer.h"

#include "d3d12definitions.h"
#include "d3d12descriptorheap.h"

namespace limbo::rhi
{
	class D3D12Buffer : public Buffer
	{
	public:
		ComPtr<ID3D12Resource> resource;

		D3D12DescriptorHandle		handle;

		D3D12_RESOURCE_STATES		currentState;

	public:
		D3D12Buffer() = default;
		D3D12Buffer(const BufferSpec& spec);

		virtual ~D3D12Buffer();
	};
}

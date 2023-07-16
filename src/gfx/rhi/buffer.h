#pragma once

#include "buffer.h"

#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::gfx
{
	enum class BufferUsage
	{
		Vertex,
		Index,
		Constant
	};

	struct BufferSpec
	{
		const char* debugName = nullptr;

		uint32		byteSize = 0;
		BufferUsage usage = BufferUsage::Constant;
		const void* initialData = nullptr;
	};


	class Buffer
	{
	public:
		ComPtr<ID3D12Resource> resource;

		DescriptorHandle		handle;

		D3D12_RESOURCE_STATES		currentState;

	public:
		Buffer() = default;
		Buffer(const BufferSpec& spec);

		virtual ~Buffer();
	};
}

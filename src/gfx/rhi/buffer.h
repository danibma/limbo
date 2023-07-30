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
		Constant,
		Upload
	};

	struct BufferSpec
	{
		const char*		debugName = nullptr;

		uint32			byteStride	= 0;
		uint64			byteSize	= 0;
		BufferUsage		usage		= BufferUsage::Constant;
		const void*		initialData = nullptr;
	};


	class Buffer
	{
	public:
		ComPtr<ID3D12Resource>	resource;
		D3D12_RESOURCE_STATES	currentState;
		D3D12_RESOURCE_STATES	initialState;
		DescriptorHandle		handle;
		void*					mappedData;

		uint32					byteStride = 0;
		uint64					byteSize = 0;

	public:
		Buffer() = default;
		Buffer(const BufferSpec& spec);

		virtual ~Buffer();
	};
}

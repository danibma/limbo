#pragma once

#include "buffer.h"

#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::Gfx
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
		const char*		DebugName = nullptr;

		uint32			ByteStride	= 0;
		uint64			ByteSize	= 0;
		BufferUsage		Usage		= BufferUsage::Constant;
		const void*		InitialData = nullptr;
	};


	class Buffer
	{
	public:
		ComPtr<ID3D12Resource>	Resource;
		D3D12_RESOURCE_STATES	CurrentState;
		D3D12_RESOURCE_STATES	InitialState;
		DescriptorHandle		BasicHandle;
		void*					MappedData;

		uint32					ByteStride = 0;
		uint64					ByteSize = 0;

	public:
		Buffer() = default;
		Buffer(const BufferSpec& spec);

		virtual ~Buffer();
	};
}

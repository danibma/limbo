#pragma once

#include "buffer.h"

#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::RHI
{
	enum class BufferUsage
	{
		Vertex					= 1 << 0,
		Index					= 1 << 1,
		Constant				= 1 << 2,
		Structured				= 1 << 3,
		Upload					= 1 << 4,
		Byte					= 1 << 5,
		AccelerationStructure	= 1 << 6,
		ShaderResourceView		= 1 << 7,
		Readback				= 1 << 8
	};

	struct BufferSpec
	{
		const char*		DebugName = "";
		uint32			NumElements = 1;
		uint32			ByteStride	= 0;
		uint64			ByteSize	= 0;
		BufferUsage		Flags		= BufferUsage::Constant;
		const void*		InitialData = nullptr;
	};

	class Buffer
	{
	public:
		RefCountPtr<ID3D12Resource>	Resource;
		D3D12_RESOURCE_STATES	InitialState;
		DescriptorHandle		CBVHandle;
		void*					MappedData;

		uint32					ByteStride = 0;
		uint64					ByteSize = 0;
		bool					bResetState;

	public:
		Buffer() = default;
		Buffer(const BufferSpec& spec);

		virtual ~Buffer();

		void Map(uint32 subresource = 0, D3D12_RANGE* range = nullptr);
		void Unmap(uint32 subresource = 0, D3D12_RANGE* range = nullptr);

	private:
		void CreateCBV(ID3D12Device* device, const BufferSpec& spec);
		void CreateSRV(ID3D12Device* device, const BufferSpec& spec);
		void CreateSRV_AS(ID3D12Device* device, const BufferSpec& spec);

		void InitResource(const BufferSpec& spec);
	};

	struct VertexBufferView
	{
		D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
		uint32						SizeInBytes;
		uint32						StrideInBytes;
		uint32						Offset;
	};

	struct IndexBufferView
	{
		D3D12_GPU_VIRTUAL_ADDRESS	BufferLocation;
		uint32						SizeInBytes;
		uint32						Offset;
	};
}

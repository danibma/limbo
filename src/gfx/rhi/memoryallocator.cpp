#include "memoryallocator.h"
#include "device.h"

namespace limbo::gfx
{
	MemoryAllocator* MemoryAllocator::ptr = nullptr;

	MemoryAllocator::~MemoryAllocator()
	{
		flushUploadBuffers();
	}

	ID3D12Resource* MemoryAllocator::createUploadBuffer(uint32 size)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		ID3D12Resource* resource;

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = size,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
													D3D12_RESOURCE_STATE_COPY_SOURCE,
													nullptr,
													IID_PPV_ARGS(&resource)));

		m_uploadBuffers.emplace_back(resource);
		return resource;
	}

	void MemoryAllocator::flushUploadBuffers()
	{
		for (ID3D12Resource* resource : m_uploadBuffers)
			resource->Release();
		m_uploadBuffers.clear();
	}
}

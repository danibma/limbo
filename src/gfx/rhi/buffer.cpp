#include "buffer.h"
#include "device.h"
#include "memoryallocator.h"

#include "utils.h"

namespace limbo::gfx
{
	Buffer::Buffer(const BufferSpec& spec)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = 1,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = flags
		};

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
													D3D12_RESOURCE_STATE_COMMON,
													nullptr,
													IID_PPV_ARGS(&resource)));
		currentState = D3D12_RESOURCE_STATE_COMMON;

		ID3D12Resource* uploadBuffer = MemoryAllocator::ptr->createUploadBuffer();

		if ((spec.debugName != nullptr) && (spec.debugName[0] != '\0'))
		{
			std::wstring wname;
			utils::StringConvert(spec.debugName, wname);
			DX_CHECK(resource->SetName(wname.c_str()));
			wname.append(L"(upload buffer)");
			DX_CHECK(uploadBuffer->SetName(wname.c_str()));
		}
	}

	Buffer::~Buffer()
	{
	}
}

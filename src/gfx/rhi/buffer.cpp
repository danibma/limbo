#include "buffer.h"
#include "device.h"
#include "memoryallocator.h"

#include "core/utils.h"

namespace limbo::gfx
{
	Buffer::Buffer(const BufferSpec& spec)
		: byteStride(spec.byteStride), byteSize(spec.byteSize)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = spec.byteSize,
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

		currentState = D3D12_RESOURCE_STATE_COMMON;
		initialState = currentState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
													initialState,
													nullptr,
													IID_PPV_ARGS(&resource)));

		if (spec.initialData)
		{
			ID3D12Resource* uploadBuffer = MemoryAllocator::ptr->createUploadBuffer(spec.byteSize);
			FAILIF(!uploadBuffer);

			void* data;
			uploadBuffer->Map(0, nullptr, &data);
			memcpy(data, spec.initialData, spec.byteSize);
			uploadBuffer->Unmap(0, nullptr);

			device->copyResource(resource.Get(), uploadBuffer);

			if ((spec.debugName != nullptr) && (spec.debugName[0] != '\0'))
			{
				std::wstring wname;
				utils::StringConvert(spec.debugName, wname);
				wname.append(L"(upload buffer)");
				DX_CHECK(uploadBuffer->SetName(wname.c_str()));
			}

			currentState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		if ((spec.debugName != nullptr) && (spec.debugName[0] != '\0'))
		{
			std::wstring wname;
			utils::StringConvert(spec.debugName, wname);
			DX_CHECK(resource->SetName(wname.c_str()));
		}
	}

	Buffer::~Buffer()
	{
	}
}

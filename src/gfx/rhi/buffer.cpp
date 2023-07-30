#include "stdafx.h"
#include "buffer.h"
#include "device.h"

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

		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (spec.usage == BufferUsage::Upload)
			heapType = D3D12_HEAP_TYPE_UPLOAD;

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = heapType,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		currentState = D3D12_RESOURCE_STATE_COMMON;
		if (spec.usage == BufferUsage::Upload)
			currentState = D3D12_RESOURCE_STATE_GENERIC_READ;
		initialState = currentState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
													initialState,
													nullptr,
													IID_PPV_ARGS(&resource)));

		if (spec.initialData)
		{
			if (spec.usage == BufferUsage::Upload)
			{
				DX_CHECK(resource->Map(0, nullptr, &mappedData));
				memcpy(mappedData, spec.initialData, spec.byteSize);
			}
			else
			{
				std::string uploadName = std::string(spec.debugName) + " (Upload Buffer)";
				Handle<Buffer> uploadBuffer = createBuffer({
					.debugName = uploadName.c_str(),
					.byteSize = spec.byteSize,
					.usage = BufferUsage::Upload,
					.initialData = spec.initialData
				});
				Buffer* allocationBuffer = ResourceManager::ptr->getBuffer(uploadBuffer);
				FAILIF(!allocationBuffer);
				Device::ptr->copyBufferToBuffer(allocationBuffer, this, spec.byteSize, 0, 0);
				destroyBuffer(uploadBuffer);
			}
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

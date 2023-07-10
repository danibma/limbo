#include "d3d12texture.h"

#include "d3d12device.h"
#include "utils.h"

namespace limbo::rhi
{
	D3D12Texture::D3D12Texture(const TextureSpec& spec)
	{
		D3D12Device* device = Device::getAs<D3D12Device>();
		ID3D12Device* d3ddevice = device->getDevice();

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if ((spec.usage & TextureUsage::Storage) == TextureUsage::Storage)
		{
			bIsUnordered = true;
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_DESC desc = {
			.Dimension = d3dTextureType(spec.type),
			.Alignment = 0,
			.Width = spec.width,
			.Height = spec.height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = d3dFormat(spec.format),
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
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

		handle = device->allocateHandle(D3D12DescriptorHeapType::SRV);

		if (bIsUnordered)
		{
			createUAV(spec, d3ddevice);
		}
		else
		{
			ensure(false);
		}

		if ((spec.debugName != nullptr) && (spec.debugName[0] != '\0'))
		{
			std::wstring wname;
			utils::StringConvert(spec.debugName, wname);
			DX_CHECK(resource->SetName(wname.c_str()));
		}
	}

	void D3D12Texture::createUAV(const TextureSpec& spec, ID3D12Device* d3ddevice)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = d3dFormat(spec.format),
		};

		if (spec.type == TextureType::Texture1D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.type == TextureType::Texture2D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0
			};
		}
		else if (spec.type == TextureType::Texture3D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = -1;
		}

		d3ddevice->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, handle.cpuHandle);
	}

	D3D12Texture::~D3D12Texture()
	{
	}
}

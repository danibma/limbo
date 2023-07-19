#include "texture.h"

#include "device.h"
#include "core/utils.h"

namespace limbo::gfx
{
	Texture::Texture(const TextureSpec& spec)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
			bIsUnordered = true;

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
			.Flags = spec.resourceFlags
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
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
													currentState, 
													nullptr,
													IID_PPV_ARGS(&resource)));

		initResource(spec, device);
	}

	Texture::Texture(ID3D12Resource* inResource, const TextureSpec& spec)
		: resource(inResource)
	{
		initResource(spec, Device::ptr);
	}

	void Texture::createUAV(const TextureSpec& spec, ID3D12Device* device)
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

		device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, handle.cpuHandle);
	}

	void Texture::createRTV(const TextureSpec& spec, ID3D12Device* device)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {
			.Format = d3dFormat(spec.format),
		};

		if (spec.type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0
			};
		}
		else if (spec.type == TextureType::Texture3D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = 0;
			desc.Texture3D.FirstWSlice = 0;
			desc.Texture3D.WSize = -1;
		}

		device->CreateRenderTargetView(resource.Get(), &desc, handle.cpuHandle);
	}

	void Texture::createDSV(const TextureSpec& spec, ID3D12Device* device)
	{
		ensure(false);
	}

	void Texture::initResource(const TextureSpec& spec, Device* device)
	{
		ID3D12Device* d3ddevice = device->getDevice();

		if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			handle = device->allocateHandle(DescriptorHeapType::RTV);
			createRTV(spec, d3ddevice);
		}
		else if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			handle = device->allocateHandle(DescriptorHeapType::DSV);
			createDSV(spec, d3ddevice);
		}
		else
		{
			handle = device->allocateHandle(DescriptorHeapType::SRV);

			if (bIsUnordered)
				createUAV(spec, d3ddevice);
			else
				ensure(false);
		}

		if ((spec.debugName != nullptr) && (spec.debugName[0] != '\0'))
		{
			std::wstring wname;
			utils::StringConvert(spec.debugName, wname);
			DX_CHECK(resource->SetName(wname.c_str()));
		}
	}

	Texture::~Texture()
	{
	}
}

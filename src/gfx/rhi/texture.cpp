#include "stdafx.h"
#include "texture.h"

#include <d3d12/d3dx12/d3dx12_resource_helpers.h>

#include "device.h"
#include "resourcemanager.h"
#include "ringbufferallocator.h"
#include "core/utils.h"

namespace limbo::gfx
{
	Texture::Texture(const TextureSpec& spec)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		D3D12_RESOURCE_FLAGS resourceFlags = spec.resourceFlags;
		if (!spec.bCreateSrv)
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

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
			.Flags = resourceFlags
		};

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		const D3D12_CLEAR_VALUE* clearValue = nullptr;
		if (spec.clearValue.Format != DXGI_FORMAT_UNKNOWN)
			clearValue = &spec.clearValue;

		if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		else if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
			currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		else
			currentState = D3D12_RESOURCE_STATE_COMMON;

		initialState = currentState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
													initialState, 
													clearValue,
													IID_PPV_ARGS(&resource)));

		initResource(spec, device);
	}

	Texture::Texture(ID3D12Resource* inResource, const TextureSpec& spec)
		: resource(inResource)
	{
		initResource(spec, Device::ptr);
	}

	void Texture::createUAV(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = format,
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

		device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, srvhandle.cpuHandle);
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
		FAILIF(spec.type == TextureType::Texture3D); // this is not a valid texture type fro DSV

		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
			.Format = d3dFormat(spec.format),
		};

		if (spec.type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
			};
		}

		device->CreateDepthStencilView(resource.Get(), &desc, handle.cpuHandle);
	}

	void Texture::initResource(const TextureSpec& spec, Device* device)
	{
		ID3D12Device* d3ddevice = device->getDevice();

		DXGI_FORMAT srvformat = d3dFormat(spec.format);
		if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			handle = device->allocateHandle(DescriptorHeapType::RTV);
			createRTV(spec, d3ddevice);
		}
		else if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			handle = device->allocateHandle(DescriptorHeapType::DSV);
			createDSV(spec, d3ddevice);
			switch (spec.format)
			{
			case Format::D16_UNORM: 
				srvformat = DXGI_FORMAT_R16_UNORM;
				break;
			case Format::D32_SFLOAT: 
				srvformat = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format::D32_SFLOAT_S8_UINT: 
				srvformat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			default:
				ensure(false);
				break;
			}
		}

		if (spec.bCreateSrv)
		{
			srvhandle = device->allocateHandle(DescriptorHeapType::SRV);

			if (spec.resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
				createUAV(spec, d3ddevice, srvformat);
			else
				createSRV(spec, d3ddevice, srvformat);
		}

		if (spec.initialData)
		{
			D3D12_RESOURCE_DESC desc = resource->GetDesc();

			uint64 size;
			d3ddevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &size);

			std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
			subresourceData.emplace_back(spec.initialData, spec.width * 4, spec.width * spec.height * 4);

			RingBufferAllocation allocation;
			ensure(RingBufferAllocator::ptr->allocate(size, allocation));

			Buffer* allocationBuffer = ResourceManager::ptr->getBuffer(allocation.buffer);
			FAILIF(!allocationBuffer);
			UpdateSubresources(device->getCommandList(), resource.Get(), allocationBuffer->resource.Get(), allocation.offset, 0, 1, subresourceData.data());

			currentState = D3D12_RESOURCE_STATE_COPY_DEST;
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

	void Texture::createSRV(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = format,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		if (spec.type == TextureType::Texture1D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.type == TextureType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.type == TextureType::Texture3D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.ResourceMinLODClamp = 0.0f
			};
		}

		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvhandle.cpuHandle);
	}
}

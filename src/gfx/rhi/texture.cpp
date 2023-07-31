#include "stdafx.h"
#include "texture.h"
#include "device.h"
#include "resourcemanager.h"
#include "core/utils.h"

namespace limbo::Gfx
{
	Texture::Texture(const TextureSpec& spec)
	{
		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		D3D12_RESOURCE_FLAGS resourceFlags = spec.ResourceFlags;
		if (!spec.bCreateSrv)
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3DTextureType(spec.Type),
			.Alignment = 0,
			.Width = spec.Width,
			.Height = spec.Height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = D3DFormat(spec.Format),
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
		if (spec.ClearValue.Format != DXGI_FORMAT_UNKNOWN)
			clearValue = &spec.ClearValue;

		if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			CurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
			CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		else
			CurrentState = D3D12_RESOURCE_STATE_COMMON;

		InitialState = CurrentState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
													InitialState, 
													clearValue,
													IID_PPV_ARGS(&Resource)));

		InitResource(spec, device);
	}

	Texture::Texture(ID3D12Resource* inResource, const TextureSpec& spec)
		: Resource(inResource)
	{
		InitResource(spec, Device::Ptr);
	}

	void Texture::CreateUav(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = format,
		};

		if (spec.Type == TextureType::Texture1D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture3D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = -1;
		}

		device->CreateUnorderedAccessView(Resource.Get(), nullptr, &uavDesc, SRVHandle.CpuHandle);
	}

	void Texture::CreateRtv(const TextureSpec& spec, ID3D12Device* device)
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {
			.Format = D3DFormat(spec.Format),
		};

		if (spec.Type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture3D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = 0;
			desc.Texture3D.FirstWSlice = 0;
			desc.Texture3D.WSize = -1;
		}

		device->CreateRenderTargetView(Resource.Get(), &desc, BasicHandle.CpuHandle);
	}

	void Texture::CreateDsv(const TextureSpec& spec, ID3D12Device* device)
	{
		FAILIF(spec.Type == TextureType::Texture3D); // this is not a valid texture type fro DSV

		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
			.Format = D3DFormat(spec.Format),
		};

		if (spec.Type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
			};
		}

		device->CreateDepthStencilView(Resource.Get(), &desc, BasicHandle.CpuHandle);
	}

	void Texture::InitResource(const TextureSpec& spec, Device* device)
	{
		ID3D12Device* d3ddevice = device->GetDevice();

		DXGI_FORMAT srvformat = D3DFormat(spec.Format);
		if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			BasicHandle = device->AllocateHandle(DescriptorHeapType::RTV);
			CreateRtv(spec, d3ddevice);
		}
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			BasicHandle = device->AllocateHandle(DescriptorHeapType::DSV);
			CreateDsv(spec, d3ddevice);
			switch (spec.Format)
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
			SRVHandle = device->AllocateHandle(DescriptorHeapType::SRV);

			if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
				CreateUav(spec, d3ddevice, srvformat);
			else
				CreateSrv(spec, d3ddevice, srvformat);
		}

		if (spec.InitialData)
		{
			D3D12_RESOURCE_DESC desc = Resource->GetDesc();

			uint64 size;
			d3ddevice->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, nullptr, nullptr, &size);

			std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
			subresourceData.emplace_back(spec.InitialData, spec.Width * 4, spec.Width * spec.Height * 4);

			std::string uploadName = std::string(spec.DebugName) + " (Upload Buffer)";
			uint32 rowPitch = Math::Align(spec.Width * 4, (uint32)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			Handle<Buffer> uploadBuffer = CreateBuffer({
				.DebugName = uploadName.c_str(),
				.ByteSize = rowPitch * spec.Height,
				.Usage = BufferUsage::Upload,
				.InitialData = spec.InitialData
			});
			
			Buffer* allocationBuffer = ResourceManager::Ptr->GetBuffer(uploadBuffer);
			FAILIF(!allocationBuffer);
			Device::Ptr->CopyBufferToTexture(allocationBuffer, this);
			DestroyBuffer(uploadBuffer);

			CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		if ((spec.DebugName != nullptr) && (spec.DebugName[0] != '\0'))
		{
			std::wstring wname;
			Utils::StringConvert(spec.DebugName, wname);
			DX_CHECK(Resource->SetName(wname.c_str()));
		}
	}

	Texture::~Texture()
	{
	}

	void Texture::CreateSrv(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = format,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		if (spec.Type == TextureType::Texture1D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.Type == TextureType::Texture3D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.ResourceMinLODClamp = 0.0f
			};
		}

		device->CreateShaderResourceView(Resource.Get(), &srvDesc, SRVHandle.CpuHandle);
	}
}

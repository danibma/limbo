#include "stdafx.h"
#include "texture.h"
#include "device.h"
#include "resourcemanager.h"
#include "core/utils.h"
#include "core/paths.h"

#include <stb/stb_image.h>

namespace limbo::Gfx
{
	void Texture::CreateResource(const TextureSpec& spec)
	{
		Spec = spec;

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
			.DepthOrArraySize = spec.Type == TextureType::TextureCube ? 6u : 1u,
			.MipLevels = spec.MipLevels,
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
			InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
			InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
			InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		else
			InitialState = D3D12_RESOURCE_STATE_COMMON;

		for (uint16 i = 0; i < spec.MipLevels; ++i)
			CurrentState[i] = InitialState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
			         InitialState, 
			         clearValue,
			         IID_PPV_ARGS(&Resource)));
	}

	Texture::Texture(const TextureSpec& spec)
	{
		CreateResource(spec);
		InitResource(spec);
	}

	Texture::Texture(const char* path, const char* debugName)
	{
		void* data;
		int width, height, numChannels;
		Format format;

		char extension[16];
		Paths::GetExtension(path, extension, true);

		if (strcmp(extension, ".hdr") == 0)
		{
			data = stbi_loadf(path, &width, &height, &numChannels, 4);
			FAILIF(!data);
			format = Format::RGBA32_SFLOAT;
		}
		else
		{
			data = stbi_load(path, &width, &height, &numChannels, 4);
			FAILIF(!data);
			if (stbi_is_16_bit(path))
				format = Format::RGBA16_UNORM;
			else
				format = Format::RGBA8_UNORM;
		}

		const TextureSpec spec = {
			.Width = (uint32)width,
			.Height = (uint32)height,
			.DebugName = debugName,
			.Format = format,
			.Type = TextureType::Texture2D,
			.InitialData = data,
		};
		CreateResource(spec);
		InitResource(spec);

		stbi_image_free(data);
	}

	Texture::Texture(ID3D12Resource* inResource, const TextureSpec& spec)
		: Resource(inResource)
	{
		InitResource(spec);
	}

	void Texture::CreateUav(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format, uint8 mipLevel)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM : format,
		};

		if (spec.Type == TextureType::Texture1D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D = {
				.MipSlice = mipLevel
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D = {
				.MipSlice = mipLevel,
				.PlaneSlice = 0
			};
		}
		else if (spec.Type == TextureType::Texture3D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = mipLevel;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = -1;
		}
		else if (spec.Type == TextureType::TextureCube)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray = {
				.MipSlice = mipLevel,
				.FirstArraySlice = 0,
				.ArraySize = 6,
				.PlaneSlice = 0
			};
		}

		device->CreateUnorderedAccessView(Resource.Get(), nullptr, &uavDesc, BasicHandle[mipLevel].CpuHandle);
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

		device->CreateRenderTargetView(Resource.Get(), &desc, BasicHandle[0].CpuHandle);
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

		device->CreateDepthStencilView(Resource.Get(), &desc, BasicHandle[0].CpuHandle);
	}

	void Texture::InitResource(const TextureSpec& spec)
	{
		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		DXGI_FORMAT srvformat = D3DFormat(spec.Format);
		if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			if (BasicHandle[0].CpuHandle.ptr == 0)
				BasicHandle[0] = device->AllocateHandle(DescriptorHeapType::RTV);
			CreateRtv(spec, d3ddevice);
		}
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			if (BasicHandle[0].CpuHandle.ptr == 0)
				BasicHandle[0] = device->AllocateHandle(DescriptorHeapType::DSV);
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
		else if (spec.ResourceFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		{
			for (uint8 i = 0; i < spec.MipLevels; ++i)
			{
				if (BasicHandle[i].CpuHandle.ptr == 0)
					BasicHandle[i] = device->AllocateHandle(DescriptorHeapType::SRV);
				CreateUav(spec, d3ddevice, srvformat, i);
			}
		}

		if (spec.bCreateSrv)
		{
			for (uint8 i = 0; i < spec.MipLevels; ++i)
			{
				if (i > 0 && (spec.Type == TextureType::Texture3D || spec.Type == TextureType::TextureCube))
					break;

				if (SRVHandle[i].CpuHandle.ptr == 0)
					SRVHandle[i] = device->AllocateHandle(DescriptorHeapType::SRV);
				CreateSrv(spec, d3ddevice, srvformat, i);
			}
		}

		if (spec.InitialData)
		{
			uint8 bytesPerChannel = D3DBytesPerChannel(spec.Format);
			uint8 numChannels = 4;

			std::string uploadName = std::string(spec.DebugName) + " (Upload Buffer)";
			uint32 rowPitch = Math::Align(spec.Width * numChannels, (uint32)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			Handle<Buffer> uploadBuffer = CreateBuffer({
				.DebugName = uploadName.c_str(),
				.ByteSize = rowPitch * spec.Height * bytesPerChannel,
				.Usage = BufferUsage::Upload,
				.InitialData = spec.InitialData
			});

			Buffer* allocationBuffer = ResourceManager::Ptr->GetBuffer(uploadBuffer);
			FAILIF(!allocationBuffer);
			Device::Ptr->CopyBufferToTexture(allocationBuffer, this);
			DestroyBuffer(uploadBuffer);
		}

		if (!spec.DebugName.empty())
		{
			std::wstring wname;
			Utils::StringConvert(spec.DebugName, wname);
			DX_CHECK(Resource->SetName(wname.c_str()));
		}
	}

	Texture::~Texture()
	{
	}

	void Texture::ReloadSize(uint32 width, uint32 height)
	{
		Spec.Width  = width;
		Spec.Height = height;

		Resource.Reset();

		CreateResource(Spec);
		InitResource(Spec);
	}

	void Texture::CreateSrv(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format, uint8 mipLevel)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = format,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		if (spec.Type == TextureType::Texture1D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D = {
				.MostDetailedMip = mipLevel,
				.MipLevels = spec.MipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.Type == TextureType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D = {
				.MostDetailedMip = mipLevel,
				.MipLevels = mipLevel == 0 ? spec.MipLevels : 1u,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.Type == TextureType::Texture3D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D = {
				.MostDetailedMip = mipLevel,
				.MipLevels = spec.MipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (spec.Type == TextureType::TextureCube)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube = {
				.MostDetailedMip = mipLevel,
				.MipLevels = spec.MipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}

		device->CreateShaderResourceView(Resource.Get(), &srvDesc, SRVHandle[mipLevel].CpuHandle);
	}
}

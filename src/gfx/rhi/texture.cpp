#include "stdafx.h"
#include "texture.h"
#include "device.h"
#include "resourcemanager.h"
#include "core/utils.h"
#include "core/paths.h"

#include <stb/stb_image.h>

#include "ringbufferallocator.h"

namespace limbo::Gfx
{
	void Texture::CreateResource(const TextureSpec& spec)
	{
		Spec = spec;

		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		InitialState = D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		if (EnumHasAllFlags(spec.Flags, TextureUsage::UnorderedAccess))
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		if (EnumHasAllFlags(spec.Flags, TextureUsage::RenderTarget))
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		}

		if (EnumHasAllFlags(spec.Flags, TextureUsage::DepthStencil))
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

			if (!EnumHasAllFlags(spec.Flags, TextureUsage::ShaderResource))
				resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

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
			.Flags = TextureUsage::ShaderResource,
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

	void Texture::CreateUAV(uint8 mipLevel)
	{
		Device::Ptr->CreateUAV(this, BasicHandle[mipLevel], mipLevel);
	}

	void Texture::CreateSRV()
	{
		Device::Ptr->CreateSRV(this, SRVHandle);
	}

	void Texture::CreateRTV()
	{
		D3D12_RENDER_TARGET_VIEW_DESC desc = {
			.Format = D3DFormat(Spec.Format),
		};

		if (Spec.Type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (Spec.Type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
				.PlaneSlice = 0
			};
		}
		else if (Spec.Type == TextureType::Texture3D)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = 0;
			desc.Texture3D.FirstWSlice = 0;
			desc.Texture3D.WSize = -1;
		}

		Device::Ptr->GetDevice()->CreateRenderTargetView(Resource.Get(), &desc, BasicHandle[0].CpuHandle);
	}

	void Texture::CreateDSV()
	{
		FAILIF(Spec.Type == TextureType::Texture3D); // this is not a valid texture type fro DSV

		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {
			.Format = D3DFormat(Spec.Format),
		};

		if (Spec.Type == TextureType::Texture1D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			desc.Texture1D = {
				.MipSlice = 0
			};
		}
		else if (Spec.Type == TextureType::Texture2D)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D = {
				.MipSlice = 0,
			};
		}

		Device::Ptr->GetDevice()->CreateDepthStencilView(Resource.Get(), &desc, BasicHandle[0].CpuHandle);
	}

	void Texture::InitResource(const TextureSpec& spec)
	{
		Spec = spec;

		Device* device = Device::Ptr;

		DXGI_FORMAT srvformat = D3DFormat(spec.Format);
		if (EnumHasAllFlags(spec.Flags, TextureUsage::RenderTarget))
		{
			if (BasicHandle[0].CpuHandle.ptr == 0)
				BasicHandle[0] = device->AllocateHandle(DescriptorHeapType::RTV);
			CreateRTV();
		}
		else if (EnumHasAllFlags(spec.Flags, TextureUsage::DepthStencil))
		{
			if (BasicHandle[0].CpuHandle.ptr == 0)
				BasicHandle[0] = device->AllocateHandle(DescriptorHeapType::DSV);
			CreateDSV();
		}
		else if (EnumHasAllFlags(spec.Flags, TextureUsage::UnorderedAccess))
		{
			for (uint8 i = 0; i < spec.MipLevels; ++i)
			{
				if (BasicHandle[i].CpuHandle.ptr == 0)
					BasicHandle[i] = device->AllocateHandle(DescriptorHeapType::SRV);
				CreateUAV(i);
			}
		}

		if (EnumHasAllFlags(spec.Flags, TextureUsage::ShaderResource))
		{
			if (SRVHandle.CpuHandle.ptr == 0)
				SRVHandle = device->AllocateHandle(DescriptorHeapType::SRV);
			CreateSRV();
		}

		if (spec.InitialData)
		{
			uint8 bytesPerChannel = D3DBytesPerChannel(spec.Format);
			uint8 numChannels = 4;

			uint32 rowPitch = Math::Align(spec.Width * numChannels, (uint32)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
			uint64 size = rowPitch * spec.Height * bytesPerChannel;

			RingBufferAllocation allocation;
			GetRingBufferAllocator()->Allocate(size, allocation);
			memcpy(allocation.MappedData, spec.InitialData, size);
			allocation.Context->CopyBufferToTexture(allocation.Buffer, this, allocation.Offset);
			GetRingBufferAllocator()->Free(allocation);
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
		Device::Ptr->FreeHandle(SRVHandle);
		for (uint8 i = 0; i < Spec.MipLevels; ++i)
			Device::Ptr->FreeHandle(BasicHandle[i]);
	}

	void Texture::ReloadSize(uint32 width, uint32 height)
	{
		Spec.Width  = width;
		Spec.Height = height;

		Resource.Reset();

		CreateResource(Spec);
		InitResource(Spec);
	}
}

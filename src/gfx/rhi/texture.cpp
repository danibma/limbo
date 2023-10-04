#include "stdafx.h"
#include "texture.h"
#include "device.h"
#include "resourcemanager.h"
#include "core/utils.h"
#include "core/paths.h"

#include <stb/stb_image.h>

#include "ringbufferallocator.h"

namespace limbo::RHI
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
			.Flags = TextureUsage::ShaderResource | TextureUsage::UnorderedAccess,
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
		Device::Ptr->CreateUAV(this, UAVHandle[mipLevel], mipLevel);
	}

	void Texture::CreateSRV()
	{
		Device::Ptr->CreateSRV(this, m_SRVHandle);
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

		Device::Ptr->GetDevice()->CreateRenderTargetView(Resource.Get(), &desc, UAVHandle[0].CpuHandle);
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

		Device::Ptr->GetDevice()->CreateDepthStencilView(Resource.Get(), &desc, UAVHandle[0].CpuHandle);
	}

	void Texture::InitResource(const TextureSpec& spec)
	{
		Spec = spec;
		bResetState = true;

		Device* device = Device::Ptr;

		if (EnumHasAllFlags(spec.Flags, TextureUsage::RenderTarget))
		{
			if (UAVHandle[0].CpuHandle.ptr == 0)
				UAVHandle[0] = device->AllocatePersistent(DescriptorHeapType::RTV);
			CreateRTV();
		}
		else if (EnumHasAllFlags(spec.Flags, TextureUsage::DepthStencil))
		{
			if (UAVHandle[0].CpuHandle.ptr == 0)
				UAVHandle[0] = device->AllocatePersistent(DescriptorHeapType::DSV);
			CreateDSV();
		}
		else if (EnumHasAllFlags(spec.Flags, TextureUsage::UnorderedAccess))
		{
			for (uint8 i = 0; i < spec.MipLevels; ++i)
			{
				if (UAVHandle[i].CpuHandle.ptr == 0)
					UAVHandle[i] = device->AllocatePersistent(DescriptorHeapType::UAV);
				CreateUAV(i);
			}
		}

		if (EnumHasAllFlags(spec.Flags, TextureUsage::ShaderResource))
		{
			if (m_SRVHandle.CpuHandle.ptr == 0)
				m_SRVHandle = device->AllocatePersistent(DescriptorHeapType::SRV);
			CreateSRV();
		}

		if (spec.InitialData)
		{
			uint8 bytesPerChannel = D3DBytesPerChannel(spec.Format);
			uint8 numChannels = 4;


			D3D12_SUBRESOURCE_DATA subresourceData = {
				spec.InitialData,
				spec.Width * numChannels * bytesPerChannel,
				spec.Width * spec.Height * numChannels * bytesPerChannel,
			};

			uint64 size = spec.Width * numChannels * spec.Height * bytesPerChannel;
			RingBufferAllocation allocation;
			GetRingBufferAllocator()->Allocate(size, allocation);
			UpdateSubresources(allocation.Context->Get(), Resource.Get(), allocation.Buffer->Resource.Get(), allocation.Offset, 0, 1, &subresourceData);
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
		Device::Ptr->FreeHandle(m_SRVHandle);
		for (uint8 i = 0; i < Spec.MipLevels; ++i)
			Device::Ptr->FreeHandle(UAVHandle[i]);
	}

	void Texture::ReloadSize(uint32 width, uint32 height)
	{
		Spec.Width  = width;
		Spec.Height = height;

		Resource.Reset();

		CreateResource(Spec);
		InitResource(Spec);
	}

	RHI::TextureSpec Tex2DDepth(uint32 width, uint32 height, float depthClearValue, const char* debugName /*= nullptr*/)
	{
		return {
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.DebugName = debugName ? debugName : "",
			.Flags = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
			.ClearValue = {
				.Format = D3DFormat(Format::D32_SFLOAT),
				.DepthStencil = {
					.Depth = depthClearValue,
					.Stencil = 0
				}
			},
			.Format = Format::D32_SFLOAT,
			.Type = TextureType::Texture2D,
		};
	}

	limbo::RHI::TextureSpec Tex2DRenderTarget(uint32 width, uint32 height, Format format, const char* debugName /*= nullptr*/, float4 clearValue /*= float4(0.0f)*/)
	{
		return {
			.Width = width,
			.Height = height,
			.MipLevels = 1,
			.DebugName = debugName ? debugName : "",
			.Flags = TextureUsage::RenderTarget | TextureUsage::ShaderResource,
			.ClearValue = {
				.Format = D3DFormat(format),
				.Color = clearValue[0]
			},
			.Format = format,
			.Type = TextureType::Texture2D,
		};
	}

}

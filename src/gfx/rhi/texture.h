#pragma once

#include "texture.h"
#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::Gfx
{
	struct TextureSpec
	{
	public:
		uint32					Width;
		uint32					Height;
		uint16					MipLevels = 1;
		const char*				DebugName;
		D3D12_RESOURCE_FLAGS	ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_CLEAR_VALUE		ClearValue;
		Format					Format = Format::R8_UNORM;
		TextureType				Type = TextureType::Texture2D;
		void*					InitialData;
		bool					bCreateSrv = true;
	};

	class Texture
	{
	public:
		ComPtr<ID3D12Resource>		Resource;
		D3D12_RESOURCE_STATES		CurrentState[D3D12_REQ_MIP_LEVELS];
		D3D12_RESOURCE_STATES		InitialState;
		DescriptorHandle			BasicHandle[D3D12_REQ_MIP_LEVELS];
		DescriptorHandle			SRVHandle;

	public:
		Texture() = default;
		Texture(const TextureSpec& spec);
		Texture(const char* path, const char* debugName);
		Texture(ID3D12Resource* inResource, const TextureSpec& spec);

		~Texture();

		// D3D12 Specific
		void CreateUav(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format, uint8 mipLevel);
		void CreateSrv(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format);
		void CreateRtv(const TextureSpec& spec, ID3D12Device* device);
		void CreateDsv(const TextureSpec& spec, ID3D12Device* device);

		void CreateResource(const TextureSpec& spec);
		void InitResource(const TextureSpec& spec);
	};
}

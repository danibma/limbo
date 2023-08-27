#pragma once

#include "texture.h"
#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::Gfx
{
	enum class TextureUsage
	{
		None			= 1 << 0,
		UnorderedAccess = 1 << 1,
		ShaderResource	= 1 << 2,
		RenderTarget	= 1 << 3,
		DepthStencil	= 1 << 4,
	};

	struct TextureSpec
	{
	public:
		uint32					Width;
		uint32					Height;
		uint16					MipLevels = 1;
		std::string				DebugName;
		TextureUsage			Flags = TextureUsage::None;
		D3D12_CLEAR_VALUE		ClearValue;
		Format					Format = Format::R8_UNORM;
		TextureType				Type = TextureType::Texture2D;
		void*					InitialData;
	};

	class Texture
	{
	public:
		ComPtr<ID3D12Resource>		Resource;
		D3D12_RESOURCE_STATES		InitialState;
		DescriptorHandle			BasicHandle[D3D12_REQ_MIP_LEVELS];
		DescriptorHandle			SRVHandle;
		TextureSpec					Spec;
		bool						bResetState;

	public:
		Texture() = default;
		Texture(const TextureSpec& spec);
		Texture(const char* path, const char* debugName);
		Texture(ID3D12Resource* inResource, const TextureSpec& spec);

		~Texture();

		void ReloadSize(uint32 width, uint32 height);

		// D3D12 Specific
		void CreateUAV(uint8 mipLevel);
		void CreateSRV();
		void CreateRTV();
		void CreateDSV();

		void CreateResource(const TextureSpec& spec);
		void InitResource(const TextureSpec& spec);
	};
}

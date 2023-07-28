#pragma once

#include "texture.h"
#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::gfx
{
	struct TextureSpec
	{
	public:
		uint32					width;
		uint32					height;
		const char*				debugName;
		D3D12_RESOURCE_FLAGS	resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_CLEAR_VALUE		clearValue;
		Format					format = Format::R8_UNORM;
		TextureType				type = TextureType::Texture2D;
		void*					initialData;
		bool					bCreateSrv = true;
	};

	class Texture
	{
	public:
		ComPtr<ID3D12Resource>		resource;
		D3D12_RESOURCE_STATES		currentState;
		D3D12_RESOURCE_STATES		initialState;
		DescriptorHandle			handle;
		DescriptorHandle			srvhandle;

	public:
		Texture() = default;
		Texture(const TextureSpec& spec);
		Texture(ID3D12Resource* inResource, const TextureSpec& spec);

		~Texture();

		// D3D12 Specific
		void createSRV(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format);
		void createUAV(const TextureSpec& spec, ID3D12Device* device, DXGI_FORMAT format);
		void createRTV(const TextureSpec& spec, ID3D12Device* device);
		void createDSV(const TextureSpec& spec, ID3D12Device* device);

		void resetResourceState();

		void initResource(const TextureSpec& spec, class Device* device);
	};
}

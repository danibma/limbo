#pragma once

#include "texture.h"
#include "definitions.h"
#include "descriptorheap.h"

namespace limbo::gfx
{
	struct TextureSpec
	{
		uint32					width;
		uint32					height;
		const char*				debugName;
		D3D12_RESOURCE_FLAGS	resourceFlags = D3D12_RESOURCE_FLAG_NONE;
		Format					format = Format::R8_UNORM;
		TextureType				type = TextureType::Texture2D;
	};

	class Texture
	{
	public:
		ComPtr<ID3D12Resource>		resource;
		bool						bIsUnordered = false;

		DescriptorHandle		handle;

		D3D12_RESOURCE_STATES		currentState;

	public:
		Texture() = default;
		Texture(const TextureSpec& spec);

		~Texture();

		// D3D12 Specific
		void createUAV(const TextureSpec& spec, ID3D12Device* d3ddevice);
	};
}

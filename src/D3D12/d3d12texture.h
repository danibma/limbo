#pragma once

#include "texture.h"
#include "d3d12definitions.h"
#include "d3d12descriptorheap.h"

namespace limbo::rhi
{
	class D3D12Texture final : public Texture
	{
	public:
		ComPtr<ID3D12Resource>		resource;
		bool						bIsUnordered = false;

		D3D12DescriptorHandle		handle;

		D3D12_RESOURCE_STATES		currentState;

	public:
		D3D12Texture() = default;
		D3D12Texture(const TextureSpec& spec);

		virtual ~D3D12Texture();

		// D3D12 Specific
		void createUAV(const TextureSpec& spec, ID3D12Device* d3ddevice);
	};
}

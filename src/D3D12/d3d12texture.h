#pragma once

#include "texture.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12Texture final : public Texture
	{
		ComPtr<ID3D12Resource>		m_resource;
		bool						m_bIsUnordered = false;

	public:
		D3D12Texture() = default;
		D3D12Texture(const TextureSpec& spec);

		virtual ~D3D12Texture();

		// D3D12 Specific
		bool isUnordered() const { return m_bIsUnordered; }
	};
}
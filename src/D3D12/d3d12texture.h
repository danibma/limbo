#pragma once

#include "texture.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12Texture final : public Texture
	{
	public:
		D3D12Texture() = default;
		D3D12Texture(const TextureSpec& spec);

		virtual ~D3D12Texture();
	};
}
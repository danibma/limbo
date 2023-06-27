#pragma once

#include "texture.h"

namespace limbo
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture() = default;
		VulkanTexture(const TextureSpec& spec);

		virtual ~VulkanTexture();
	};
}
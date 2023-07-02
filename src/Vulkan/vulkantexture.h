#pragma once

#include "texture.h"
#include "vulkandefinitions.h"

namespace limbo
{
	class VulkanTexture final : public Texture
	{
	private:
		VkImage			m_image;
		VkImageView		m_imageView;

	public:
		VulkanTexture() = default;
		VulkanTexture(const TextureSpec& spec);

		virtual ~VulkanTexture();
	};
}
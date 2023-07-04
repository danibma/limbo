#pragma once

#include "texture.h"
#include "vulkandefinitions.h"

namespace limbo::rhi
{
	class VulkanTexture final : public Texture
	{
	public:
		VkImage			image;
		VkImageView		imageView;
		VkImageLayout	layout;
		VkDeviceMemory	memory;

	public:
		VulkanTexture() = default;
		VulkanTexture(const TextureSpec& spec);

		virtual ~VulkanTexture();
	};
}
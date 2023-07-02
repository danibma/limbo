#include "vulkantexture.h"

#include "vulkandevice.h"

namespace limbo::rhi
{
	VulkanTexture::VulkanTexture(const TextureSpec& spec)
	{
		ensure(spec.width  > 0);
		ensure(spec.height > 0);

		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = vkImageType(spec.type),
			.format = vkFormat(spec.format),
			.extent = {.width = spec.width, .height = spec.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VK_CHECK(vk::vkCreateImage(device->getDevice(), &imageInfo, nullptr, &m_image));
	}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		vk::vkDestroyImage(device->getDevice(), m_image, nullptr);
	}
}

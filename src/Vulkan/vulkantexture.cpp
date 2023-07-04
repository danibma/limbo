#include "vulkantexture.h"

#include "vulkandevice.h"

namespace limbo::rhi
{
	VulkanTexture::VulkanTexture(const TextureSpec& spec)
	{
		ensure(spec.width  > 0);
		ensure(spec.height > 0);

		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkDevice vkDevice = device->getDevice();

		layout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = vkImageType(spec.type),
			.format = vkFormat(spec.format),
			.extent = {.width = spec.width, .height = spec.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = VK_IMAGE_USAGE_STORAGE_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = layout
		};
		VK_CHECK(vk::vkCreateImage(vkDevice, &imageInfo, nullptr, &image));

		VkMemoryRequirements memoryRequirements;
		vk::vkGetImageMemoryRequirements(vkDevice, image, &memoryRequirements);

		uint32_t memoryTypeIndex = device->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		ensure(memoryTypeIndex != ~0u);

		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = memoryTypeIndex;

		VK_CHECK(vk::vkAllocateMemory(vkDevice, &allocateInfo, 0, &memory));

		VK_CHECK(vk::vkBindImageMemory(vkDevice, image, memory, 0));

		VkImageViewCreateInfo imageViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = vkImageViewType(spec.type),
			.format = vkFormat(spec.format),
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		VK_CHECK(vk::vkCreateImageView(vkDevice, &imageViewInfo, nullptr, &imageView));

		layout = VK_IMAGE_LAYOUT_GENERAL;
		VkImageMemoryBarrier2 imageBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = layout,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		VkDependencyInfo info = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageBarrier
		};

		device->submitPipelineBarrier(info);
	}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkDevice vkDevice = device->getDevice();

		vk::vkDestroyImageView(vkDevice, imageView, nullptr);
		vk::vkDestroyImage(vkDevice, image, nullptr);
	}
}

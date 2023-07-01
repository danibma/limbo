#include "vulkanswapchain.h"
#include "vulkandevice.h"

#include <Windows.h>

#include "limbo.h"

namespace limbo
{
	VulkanSwapchain::VulkanSwapchain(VkDevice device, VkInstance instance, VkPhysicalDevice gpu, const WindowInfo& info)
	{
		VkWin32SurfaceCreateInfoKHR surfaceInfo = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = info.hwnd
		};
		VK_CHECK(vk::vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &m_surface));

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vk::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &surfaceCapabilities);
		ensure(NUM_BUFFERS > surfaceCapabilities.minImageCount);
		ensure(NUM_BUFFERS < surfaceCapabilities.maxImageCount);

		uint32 formatCount;
		VK_CHECK(vk::vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		VK_CHECK(vk::vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, formats.data()));

		bool bFoundFormat = false;
		for (const VkSurfaceFormatKHR& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				m_surfaceFormat = format;
				bFoundFormat = true;
			}
			else if (format.format == VK_FORMAT_R8G8B8A8_UNORM)
			{
				m_surfaceFormat = format;
				bFoundFormat = true;
			}

			if (bFoundFormat) break;
		}
		ensure(bFoundFormat);

		VkSwapchainCreateInfoKHR swapchainInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = m_surface,
			.minImageCount = NUM_BUFFERS,
			.imageFormat = m_surfaceFormat.format,
			.imageColorSpace = m_surfaceFormat.colorSpace,
			.imageExtent = { .width = info.width, .height = info.height },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE ,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = true,
			.oldSwapchain = nullptr
		};

		VK_CHECK(vk::vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &m_swapchain));

		uint32 imagesCount;
		VK_CHECK(vk::vkGetSwapchainImagesKHR(device, m_swapchain, &imagesCount, nullptr));
		m_images.reserve(imagesCount);
		VK_CHECK(vk::vkGetSwapchainImagesKHR(device, m_swapchain, &imagesCount, m_images.data()));
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();

		vk::vkDestroySwapchainKHR(device->getDevice(), m_swapchain, nullptr);
		vk::vkDestroySurfaceKHR(device->getInstance(), m_surface, nullptr);
	}
}

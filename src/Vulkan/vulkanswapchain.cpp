#include "vulkanswapchain.h"
#include "vulkandevice.h"

#include <Windows.h>

#include "limbo.h"

namespace limbo::rhi
{
	VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, const limbo::WindowInfo& info)
		:m_device(device)
	{
		VkInstance instance = m_device->getInstance();
		VkDevice vkDevice = m_device->getDevice();
		VkPhysicalDevice gpu = m_device->getGPU();

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

		VK_CHECK(vk::vkCreateSwapchainKHR(vkDevice, &swapchainInfo, nullptr, &m_swapchain));

		uint32 imagesCount;
		VK_CHECK(vk::vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imagesCount, nullptr));
		m_images.resize(imagesCount);
		VK_CHECK(vk::vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imagesCount, m_images.data()));
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		vk::vkDestroySwapchainKHR(m_device->getDevice(), m_swapchain, nullptr);
		vk::vkDestroySurfaceKHR(m_device->getInstance(), m_surface, nullptr);
	}

	void VulkanSwapchain::present(const VulkanPerFrame& frame, VkQueue queue)
	{
		VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &frame.m_acquireSemaphore,
			.pWaitDstStageMask = &submitStageMask,
			.commandBufferCount = 1,
			.pCommandBuffers = &frame.m_commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &frame.m_renderSemaphore
		};
		VK_CHECK(vk::vkQueueSubmit(queue, 1, &submitInfo, nullptr));

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &frame.m_renderSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &m_swapchain,
			.pImageIndices = &m_imageIndex,
		};
		VK_CHECK(vk::vkQueuePresentKHR(queue, &presentInfo));
	}

	void VulkanSwapchain::prepareNextImage(const VulkanPerFrame& frame)
	{
		VK_CHECK(vk::vkAcquireNextImageKHR(m_device->getDevice(), m_swapchain, ~0, frame.m_acquireSemaphore, nullptr, &m_imageIndex));

		VkImageMemoryBarrier2 imageBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
			.dstAccessMask = VK_ACCESS_2_NONE,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.image = m_images[m_imageIndex],
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
		vk::vkCmdPipelineBarrier2(frame.m_commandBuffer, &info);
	}
}

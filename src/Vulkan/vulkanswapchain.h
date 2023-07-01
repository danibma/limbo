#pragma once

#include "vulkandefinitions.h"

#include <vector>

namespace limbo
{
	struct WindowInfo;
	class VulkanSwapchain
	{
	private:
		VkSurfaceKHR			m_surface;
		VkSwapchainKHR			m_swapchain;
		VkSurfaceFormatKHR		m_surfaceFormat;
		std::vector<VkImage>	m_images;

		const uint8				NUM_BUFFERS = 3;

	public:
		VulkanSwapchain(VkDevice device, VkInstance instance, VkPhysicalDevice gpu, const WindowInfo& info);
		~VulkanSwapchain();
	};
}

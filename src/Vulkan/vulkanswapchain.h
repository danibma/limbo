#pragma once

#include "vulkandefinitions.h"

#include <vector>

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	struct VulkanPerFrame;
	class VulkanDevice;
	class VulkanSwapchain
	{
	private:
		VulkanDevice*			m_device;
		VkSurfaceKHR			m_surface;
		VkSwapchainKHR			m_swapchain;
		VkSurfaceFormatKHR		m_surfaceFormat;
		std::vector<VkImage>	m_images;

		const uint8				NUM_BUFFERS = 3;

		uint32					m_imageIndex;

	public:
		VulkanSwapchain(VulkanDevice* device, const limbo::WindowInfo& info);
		~VulkanSwapchain();

		void present(const VulkanPerFrame& frame, VkQueue queue);
		void prepareNextImage(const VulkanPerFrame& frame);
	};
}

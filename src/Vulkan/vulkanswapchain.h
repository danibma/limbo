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
		VulkanDevice*				m_device;
		VkSurfaceKHR				m_surface;
		VkSwapchainKHR				m_swapchain;
		VkSurfaceFormatKHR			m_surfaceFormat;
		std::vector<VkImage>		m_images;
		std::vector<VkImageView>	m_imageViews;

		const uint8					NUM_BUFFERS = 3;

		uint32						m_imageIndex;

		uint32						m_imagesWidth;
		uint32						m_imagesHeight;

	public:
		VulkanSwapchain(VulkanDevice* device, const limbo::WindowInfo& info);
		~VulkanSwapchain();

		void present(const VulkanPerFrame& frame, VkQueue queue);
		void prepareNextImage(const VulkanPerFrame& frame);

		uint32 getImagesWidth() { return m_imagesWidth; }
		uint32 getImagesHeight() { return m_imagesHeight; }
		VkImageView getCurrentImageView() { return m_imageViews[m_imageIndex]; }
	};
}

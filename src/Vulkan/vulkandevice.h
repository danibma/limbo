#pragma once

#include "device.h"
#include "vulkanloader.h"

#include <vector>

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	struct VulkanPerFrame
	{
		VkCommandPool		m_commandPool;
		VkCommandBuffer		m_commandBuffer;
		VkSemaphore			m_acquireSemaphore;
		VkSemaphore			m_renderSemaphore;

		void destroy(VkDevice device)
		{
			vk::vkFreeCommandBuffers(device, m_commandPool, 1, &m_commandBuffer);
			vk::vkDestroyCommandPool(device, m_commandPool, nullptr);
			vk::vkDestroySemaphore(device, m_acquireSemaphore, nullptr);
			vk::vkDestroySemaphore(device, m_renderSemaphore, nullptr);
		}
	};

	class VulkanSwapchain;
	class VulkanDevice final : public Device
	{
	private:
		std::vector<VkValidationFeatureEnableEXT>	m_validationFeatures;
		std::vector<const char*>					m_instanceExtensions;
		std::vector<const char*>					m_instanceLayers;
		std::vector<const char*>					m_deviceExtensions;

		VkInstance									m_instance;
		VkPhysicalDevice							m_gpu;
		VkDevice									m_device;
		VkDebugUtilsMessengerEXT					m_messenger;
		VkPhysicalDeviceMemoryProperties			m_memoryProperties;

		VkDescriptorPool							m_descriptorPool;

		VulkanPerFrame								m_frame;

		uint32										m_graphicsQueueFamily = ~0;
		uint32										m_computeQueueFamily  = ~0;
		uint32										m_transferQueueFamily = ~0;

		VkQueue										m_graphicsQueue;

		VulkanSwapchain*							m_swapchain;

	public:
		VulkanDevice(const WindowInfo& info);
		virtual ~VulkanDevice();

		virtual void copyTextureToBackBuffer(Handle<Texture> texture) override;

		virtual void bindDrawState(const DrawInfo& drawState) override;
		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) override;

		virtual void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

		virtual void present() override;

		// Vulkan specific
		[[nodiscard]] VkDevice getDevice() { return m_device; }
		[[nodiscard]] VkInstance getInstance() { return m_instance; }
		[[nodiscard]] VkPhysicalDevice getGPU() { return m_gpu; }
		[[nodiscard]] VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }
		[[nodiscard]] uint32 getMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags flags);

		void submitPipelineBarrier(const VkDependencyInfo& info);

	private:
		void findPhysicalDevice();
		void createLogicalDevice();
		void resetFrame(const VulkanPerFrame& frame);

	};
}

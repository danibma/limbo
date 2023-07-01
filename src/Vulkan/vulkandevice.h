#pragma once

#include "device.h"
#include "vulkanloader.h"

#include <vector>

namespace limbo
{
	struct WindowInfo;
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

		uint32										m_graphicsQueueFamily = ~0;
		uint32										m_computeQueueFamily  = ~0;
		uint32										m_transferQueueFamily = ~0;

		VulkanSwapchain*							m_swapchain;

	public:
		VulkanDevice(const WindowInfo& info);
		virtual ~VulkanDevice();

		virtual void setParameter(Handle<Shader> shader, uint8 slot, const void* data) override;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer) override;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture) override;

		virtual void bindShader(Handle<Shader> shader) override;
		virtual void bindVertexBuffer(Handle<Buffer> vertexBuffer) override;
		virtual void bindIndexBuffer(Handle<Buffer> indexBuffer) override;

		virtual void copyTextureToBackBuffer(Handle<Texture> texture) override;

		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) override;

		virtual void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

		virtual void present() override;

		// Vulkan specific
		[[nodiscard]] VkDevice getDevice() { return m_device; }
		[[nodiscard]] VkInstance getInstance() { return m_instance; }

	private:
		void findPhysicalDevice();
		void createLogicalDevice();

	};
}
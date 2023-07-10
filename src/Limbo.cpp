#include "limbo.h"

// Vk
#include "Vulkan/vulkandevice.h"
#include "Vulkan/vulkanresourcemanager.h"

// D3D12
#include "D3D12/d3d12device.h"
#include "D3D12/d3d12memoryallocator.h"
#include "D3D12/d3d12resourcemanager.h"

namespace limbo
{
	Device* Device::ptr = nullptr;
	ResourceManager* ResourceManager::ptr = nullptr;

	void init(const WindowInfo&& info)
	{
		if (info.api == RHI_API::D3D12)
		{
			Device::ptr = new rhi::D3D12Device(info);
			ResourceManager::ptr = new rhi::D3D12ResourceManager();
		}
		else if (info.api == RHI_API::Vulkan)
		{
			Device::ptr = new rhi::VulkanDevice(info);
			ResourceManager::ptr = new rhi::VulkanResourceManager();
		}
	}

	void shutdown()
	{
		delete ResourceManager::ptr;
		ResourceManager::ptr = nullptr;

		delete Device::ptr;
		Device::ptr = nullptr;
	}
}

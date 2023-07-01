#include "limbo.h"

#include "Vulkan/vulkandevice.h"
#include "Vulkan/vulkanresourcemanager.h"

namespace limbo
{
	Device* Device::ptr = nullptr;
	ResourceManager* ResourceManager::ptr = nullptr;

	void init(WindowInfo info)
	{
		Device::ptr = new VulkanDevice(info);
		ResourceManager::ptr = new VulkanResourceManager();
	}

	void shutdown()
	{
		delete ResourceManager::ptr;
		ResourceManager::ptr = nullptr;

		delete Device::ptr;
		Device::ptr = nullptr;
	}
}

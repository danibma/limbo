#include "limbo.h"

#include "Vulkan/vulkandevice.h"
#include "Vulkan/vulkanresourcemanager.h"

namespace limbo
{
	Device* Device::ptr = nullptr;
	ResourceManager* ResourceManager::ptr = nullptr;

#if LIMBO_WINDOWS
	void init(HWND window)
	{
		Device::ptr = new VulkanDevice();
		ResourceManager::ptr = new VulkanResourceManager();
	}
#elif LIMBO_LINUX
	void init(Window window)
	{

	}
#endif

	void shutdown()
	{
		delete ResourceManager::ptr;
		ResourceManager::ptr = nullptr;

		delete Device::ptr;
		Device::ptr = nullptr;
	}

	Handle<Buffer> createBuffer(const BufferSpec& spec)
	{
		return ResourceManager::ptr->createBuffer(spec);
	}

}

#include "gfx.h"

#include "rhi/device.h"
#include "rhi/resourcemanager.h"

namespace limbo::gfx
{
	Device* Device::ptr = nullptr;
	ResourceManager* ResourceManager::ptr = nullptr;

	void init(const WindowInfo&& info)
	{
		Device::ptr = new Device(info);
		ResourceManager::ptr = new ResourceManager();
	}

	void shutdown()
	{
		delete ResourceManager::ptr;
		ResourceManager::ptr = nullptr;

		delete Device::ptr;
		Device::ptr = nullptr;
	}
}

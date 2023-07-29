#include "gfx.h"

#include "rhi/device.h"
#include "rhi/resourcemanager.h"

namespace limbo::gfx
{
	Device* Device::ptr = nullptr;
	ResourceManager* ResourceManager::ptr = nullptr;

	void init(core::Window* window, GfxDeviceFlags flags)
	{
		Device::ptr = new Device(window, flags);
		ResourceManager::ptr = new ResourceManager();

		onPostResourceManagerInit.Broadcast();
	}

	void shutdown()
	{
		onPreResourceManagerShutdown.Broadcast();

		delete ResourceManager::ptr;
		ResourceManager::ptr = nullptr;

		delete Device::ptr;
		Device::ptr = nullptr;
	}
}

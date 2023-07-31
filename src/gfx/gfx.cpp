#include "stdafx.h"
#include "gfx.h"

#include "rhi/device.h"
#include "rhi/resourcemanager.h"

namespace limbo::Gfx
{
	Device* Device::Ptr = nullptr;
	ResourceManager* ResourceManager::Ptr = nullptr;

	void Init(Core::Window* window, GfxDeviceFlags flags)
	{
		Device::Ptr = new Device(window, flags);
		ResourceManager::Ptr = new ResourceManager();

		OnPostResourceManagerInit.Broadcast();
	}

	void Shutdown()
	{
		OnPreResourceManagerShutdown.Broadcast();

		delete ResourceManager::Ptr;
		ResourceManager::Ptr = nullptr;

		delete Device::Ptr;
		Device::Ptr = nullptr;
	}
}

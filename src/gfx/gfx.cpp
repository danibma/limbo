#include "stdafx.h"
#include "gfx.h"

#include "rhi/device.h"
#include "rhi/resourcemanager.h"

limbo::RHI::Device* limbo::RHI::Device::Ptr = nullptr;
limbo::RHI::ResourceManager* limbo::RHI::ResourceManager::Ptr = nullptr;

namespace limbo::Gfx
{
	void Init(Core::Window* window, GfxDeviceFlags flags)
	{
		RHI::Device::Ptr = new RHI::Device(window, flags);
		RHI::ResourceManager::Ptr = new RHI::ResourceManager();

		OnPostResourceManagerInit.Broadcast();
	}

	void Shutdown()
	{
		OnPreResourceManagerShutdown.Broadcast();

		delete RHI::ResourceManager::Ptr;
		RHI::ResourceManager::Ptr = nullptr;

		delete RHI::Device::Ptr;
		RHI::Device::Ptr = nullptr;
	}
}

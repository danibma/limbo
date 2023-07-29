#pragma once

#include "core/core.h"

#include <CppDelegates/Delegates.h>

namespace limbo::core
{
	class Window;
}

namespace limbo::gfx
{
	enum GfxDeviceFlag
	{
		DetailedLogging = 1 << 0,
		EnableImgui		= 1 << 1
	};
	typedef uint8 GfxDeviceFlags;

	// init callbacks
	DECLARE_MULTICAST_DELEGATE(OnPostResourceManagerInit);
	inline OnPostResourceManagerInit onPostResourceManagerInit;

	DECLARE_MULTICAST_DELEGATE(OnPreResourceManagerShutdown);
	inline OnPreResourceManagerShutdown onPreResourceManagerShutdown;

	// init functions
	void init(core::Window* window, GfxDeviceFlags flags = 0);
	void shutdown();
}

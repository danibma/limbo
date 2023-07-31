#pragma once

#include "core/core.h"

#include <CppDelegates/Delegates.h>

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	enum GfxDeviceFlag
	{
		DetailedLogging = 1 << 0,
		EnableImgui		= 1 << 1,
		DisableVSync	= 1 << 2,
	};
	typedef uint8 GfxDeviceFlags;

	// init callbacks
	DECLARE_MULTICAST_DELEGATE(TOnPostResourceManagerInit);
	inline TOnPostResourceManagerInit OnPostResourceManagerInit;

	DECLARE_MULTICAST_DELEGATE(TOnPreResourceManagerShutdown);
	inline TOnPreResourceManagerShutdown OnPreResourceManagerShutdown;

	// init functions
	void Init(Core::Window* window, GfxDeviceFlags flags = 0);
	void Shutdown();
}

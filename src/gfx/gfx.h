#pragma once

#include "core/core.h"

namespace limbo::core
{
	class Window;
}

namespace limbo::gfx
{
	enum GfxDeviceFlag
	{
		DetailedLogging = 1 << 0
	};
	typedef uint8 GfxDeviceFlags;

	// init functions
	void init(core::Window* window, GfxDeviceFlags flags = 0);

	void shutdown();
}

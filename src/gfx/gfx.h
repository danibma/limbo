#pragma once

#include "core/core.h"

namespace limbo::gfx
{
	enum GfxDeviceFlag
	{
		DetailedLogging = 1 << 0
	};
	typedef uint8 GfxDeviceFlags;

	struct WindowInfo
	{
		HWND			hwnd;
		uint32			width;
		uint32			height;
		GfxDeviceFlags	flags = 0;
	};

	// init functions
	void init(const WindowInfo&& info);

	void shutdown();
}
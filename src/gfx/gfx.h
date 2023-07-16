#pragma once

#include "core.h"

namespace limbo::gfx
{
	struct WindowInfo
	{
		HWND	hwnd;
		uint32  width;
		uint32  height;
	};

	// init functions
	void init(const WindowInfo&& info);

	void shutdown();
}
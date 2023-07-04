#pragma once

#include "core.h"
#include "device.h"
#include "resourcemanager.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "draw.h"

#if LIMBO_LINUX
	#include <X11/Xlib.h>
#endif

namespace limbo
{
	struct WindowInfo
	{
#if LIMBO_WINDOWS
		HWND hwnd;
#elif LIMBO_LINUX
		Window window;
#endif
		uint32 width;
		uint32 height;
	};

	// init functions
	void init(WindowInfo window);

	void shutdown();
}
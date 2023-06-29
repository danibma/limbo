#pragma once

#include "core.h"
#include "device.h"
#include "resourcemanager.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"

#if LIMBO_LINUX
	#include <X11/Xlib.h>
#endif

namespace limbo
{
	// init functions
#if LIMBO_WINDOWS
	void init(HWND window);
#elif LIMBO_LINUX
	void init(Window window);
#endif

	void shutdown();
}
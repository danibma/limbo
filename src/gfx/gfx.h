#pragma once

#include "core/core.h"

#include <map>
#include <variant>

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	// Types
	enum GfxDeviceFlag
	{
		DetailedLogging = 1 << 0,
		EnableImgui		= 1 << 1,
	};
	typedef uint8 GfxDeviceFlags;

	// Callbacks
	DECLARE_MULTICAST_DELEGATE(TOnPostResourceManagerInit);
	inline TOnPostResourceManagerInit OnPostResourceManagerInit;

	DECLARE_MULTICAST_DELEGATE(TOnPreResourceManagerShutdown);
	inline TOnPreResourceManagerShutdown OnPreResourceManagerShutdown;

	// Initialization and shutdown functions
	void Init(Core::Window* window, GfxDeviceFlags flags = 0);
	void Shutdown();
}

#define SETUP_RENDER_TECHNIQUE(TechniqueClass, List) List.push_back(std::make_unique<TechniqueClass>())

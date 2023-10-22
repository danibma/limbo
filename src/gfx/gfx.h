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

	struct RenderOptions
	{
		using Option = std::variant<bool, float, uint32>;

		std::map<std::string_view, Option> Options;
	};
	using OptionsList = std::map<std::string_view, limbo::Gfx::RenderOptions::Option>;

	// Callbacks
	DECLARE_MULTICAST_DELEGATE(TOnPostResourceManagerInit);
	inline TOnPostResourceManagerInit OnPostResourceManagerInit;

	DECLARE_MULTICAST_DELEGATE(TOnPreResourceManagerShutdown);
	inline TOnPreResourceManagerShutdown OnPreResourceManagerShutdown;

	// Initialization and shutdown functions
	void Init(Core::Window* window, GfxDeviceFlags flags = 0);
	void Shutdown();
}

template<typename T> void _GetRenderOptions(T& option, const std::string_view& optionName, const limbo::Gfx::OptionsList& optionsList)
{
	option = std::get<T>(optionsList.at(optionName));
}

#define RENDER_OPTION_MAKE(options, variable) #variable, options.variable
#define RENDER_OPTION_GET(options, variable, optionsList) _GetRenderOptions(options.variable, #variable, optionsList)

#define SETUP_RENDER_TECHNIQUE(TechniqueClass, List) List.push_back(std::make_unique<TechniqueClass>())

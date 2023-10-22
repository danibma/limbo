#pragma once
#include "rendercontext.h"

namespace limbo::UI
{
	namespace Globals
	{
		inline bool bShowProfiler = false;
		inline bool bOrderProfilerResults = false;
		inline bool bDebugShadowMaps = false;
		inline int  ShadowCascadeIndex = 0;
		inline bool bShowShadowCascades = false;
	};

	void Render(Gfx::RenderContext& context, float dt);
}

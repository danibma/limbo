#pragma once

namespace limbo::Gfx
{
	class SceneRenderer;
}

namespace limbo::UI
{
	namespace Globals
	{
		inline bool bShowProfiler = false;
		inline bool bDebugShadowMaps = false;
		inline int  ShadowCascadeIndex = 0;
		inline bool bShowShadowCascades = false;
	};

	void Render(Gfx::SceneRenderer* sceneRenderer, float dt);
}

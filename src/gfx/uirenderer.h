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
	};

	void Render(Gfx::SceneRenderer* sceneRenderer, float dt);
}

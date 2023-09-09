#pragma once
#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class SceneRenderer;
	class Shader;
	class ShadowMapping
	{
		Handle<Shader>			m_ShadowMapShader;

	public:
		explicit ShadowMapping();
		~ShadowMapping();

		void BindShadowMap(Handle<Shader> shader);

		void Render(SceneRenderer* sceneRenderer);
		void DrawDebugWindow();
	};
}

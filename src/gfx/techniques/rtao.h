#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class SceneRenderer;
	class Texture;
	class Shader;
	class RTAO
	{
		Handle<Shader>				m_RTAOShader;
		Handle<Texture>				m_FinalTexture;

	public:
		explicit RTAO();
		~RTAO();

		void Render(SceneRenderer* sceneRenderer);

		Handle<Texture> GetFinalTexture() const;
	};
}


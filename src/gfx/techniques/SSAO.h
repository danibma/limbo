#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class SceneRenderer;
	class Texture;
	class Shader;

	class SSAO
	{
		// SSAO
		Handle<Shader>				m_SSAOShader;
		Handle<Texture>				m_UnblurredSSAOTexture;
		Handle<Shader>				m_BlurSSAOShader;
		Handle<Texture>				m_BlurredSSAOTexture;

	public:
		explicit SSAO();
		~SSAO();

		void Render(SceneRenderer* sceneRenderer, Handle<Texture> positionsMap, Handle<Texture> sceneDepthMap);

		Handle<Texture> GetBlurredTexture() const;
	};
}

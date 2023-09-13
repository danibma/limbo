﻿#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	class Texture;
	class Shader;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class SSAO
	{
		// SSAO
		RHI::Handle<RHI::Shader>			m_SSAOShader;
		RHI::Handle<RHI::Texture>			m_UnblurredSSAOTexture;
		RHI::Handle<RHI::Shader>			m_BlurSSAOShader;
		RHI::Handle<RHI::Texture>			m_BlurredSSAOTexture;

	public:
		explicit SSAO();
		~SSAO();

		void Render(SceneRenderer* sceneRenderer, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> sceneDepthMap);

		RHI::Handle<RHI::Texture> GetBlurredTexture() const;
	};
}

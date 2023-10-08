﻿#pragma once

#include "gfx/rhi/shader.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/texture.h"

namespace limbo::RHI
{
	class RootSignature;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class SSAO
	{
		RHI::RootSignature*		m_SSAORS;
		RHI::RootSignature*		m_BlurSSAORS;

		RHI::ShaderHandle		m_SSAOShader;
		RHI::PSOHandle			m_SSAOPSO;
		RHI::TextureHandle		m_UnblurredSSAOTexture;
		RHI::ShaderHandle		m_BlurSSAOShader;
		RHI::PSOHandle			m_BlurSSAOPSO;
		RHI::TextureHandle		m_BlurredSSAOTexture;

	public:
		explicit SSAO();
		~SSAO();

		void Render(SceneRenderer* sceneRenderer, RHI::TextureHandle positionsMap, RHI::TextureHandle sceneDepthMap);

		RHI::TextureHandle GetBlurredTexture() const;
	};
}

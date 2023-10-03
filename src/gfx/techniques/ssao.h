#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	struct Shader;
	class RootSignature;
	class PipelineStateObject;
	class Texture;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class SSAO
	{
		RHI::RootSignature*					m_SSAORS;
		RHI::RootSignature*					m_BlurSSAORS;

		RHI::Handle<RHI::Shader>			m_SSAOShader;
		RHI::PipelineStateObject*			m_SSAOPSO;
		RHI::Handle<RHI::Texture>			m_UnblurredSSAOTexture;
		RHI::Handle<RHI::Shader>			m_BlurSSAOShader;
		RHI::PipelineStateObject*			m_BlurSSAOPSO;
		RHI::Handle<RHI::Texture>			m_BlurredSSAOTexture;

	public:
		explicit SSAO();
		~SSAO();

		void Render(SceneRenderer* sceneRenderer, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> sceneDepthMap);

		RHI::Handle<RHI::Texture> GetBlurredTexture() const;
	};
}

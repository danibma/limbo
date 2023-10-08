#pragma once

#include "gfx/rhi/shader.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/texture.h"

namespace limbo::RHI
{
	class AccelerationStructure;
	class RootSignature;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class RTAO
	{
		RHI::RootSignature*	m_CommonRS;

		RHI::ShaderHandle	m_RTAOShader;
		RHI::PSOHandle		m_RTAOPSO;
		RHI::ShaderHandle	m_DenoiseRTAOShader;
		RHI::PSOHandle		m_RTAODenoisePSO;
		RHI::TextureHandle	m_NoisedTexture;
		RHI::TextureHandle	m_FinalTexture;
		RHI::TextureHandle	m_PreviousFrame;

		uint32				m_AccumCount;

	public:
		explicit RTAO();
		~RTAO();

		void Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::TextureHandle positionsMap, RHI::TextureHandle normalsMap);

		RHI::TextureHandle GetFinalTexture() const;

	private:
		void PreparePreviousFrameTexture();
	};
}


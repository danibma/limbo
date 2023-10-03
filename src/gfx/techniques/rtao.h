#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	class PipelineStateObject;
	class AccelerationStructure;
	class RootSignature;
	class Texture;
	struct Shader;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class RTAO
	{
		RHI::RootSignature*			m_CommonRS;

		RHI::Handle<RHI::Shader>	m_RTAOShader;
		RHI::PipelineStateObject*	m_RTAOPSO;
		RHI::Handle<RHI::Shader>	m_DenoiseRTAOShader;
		RHI::PipelineStateObject*	m_RTAODenoisePSO;
		RHI::Handle<RHI::Texture>	m_NoisedTexture;
		RHI::Handle<RHI::Texture>	m_FinalTexture;
		RHI::Handle<RHI::Texture>	m_PreviousFrame;

		uint32						m_AccumCount;

	public:
		explicit RTAO();
		~RTAO();

		void Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> normalsMap);

		RHI::Handle<RHI::Texture> GetFinalTexture() const;

	private:
		void PreparePreviousFrameTexture();
	};
}


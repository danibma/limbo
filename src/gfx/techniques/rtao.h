#pragma once

#include "gfx/rhi/texture.h"

namespace limbo::RHI
{
	class AccelerationStructure;
	class RootSignature;
	class CommandContext;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class RTAO
	{
		RHI::TextureHandle			m_NoisedTexture;
		RHI::TextureHandle			m_FinalTexture;
		RHI::TextureHandle			m_PreviousFrame;

		uint32						m_AccumCount;

	public:
		explicit RTAO();
		~RTAO();

		void Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::TextureHandle positionsMap, RHI::TextureHandle normalsMap);

		RHI::TextureHandle GetFinalTexture() const;

	private:
		void PreparePreviousFrameTexture();
	};
}


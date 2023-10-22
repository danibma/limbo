#pragma once

#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::RHI
{
	class AccelerationStructure;
	class RootSignature;
}

namespace limbo::Gfx
{
	class RTAO : public RenderTechnique
	{
		RHI::TextureHandle			m_NoisedTexture;
		RHI::TextureHandle			m_PreviousFrame;

		uint32						m_AccumCount;

	public:
		RTAO();
		virtual ~RTAO() override;

		virtual bool Init() override;
		virtual bool ConditionalRender(RenderContext& context) override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;

	private:
		void PreparePreviousFrameTexture();
	};
}


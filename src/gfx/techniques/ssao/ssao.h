#pragma once

#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class SSAO : public RenderTechnique
	{
		RHI::TextureHandle			m_UnblurredSSAOTexture;

	public:
		SSAO();
		virtual ~SSAO() override;

		virtual bool Init() override;
		virtual bool ConditionalRender(RenderContext& context) override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
	};
}

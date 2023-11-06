#pragma once

#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class SSAO : public RenderTechnique
	{
		RHI::TextureHandle m_UnblurredSSAOTexture;
		RHI::TextureHandle m_FinalSSAOTexture;

	public:
		SSAO();
		virtual ~SSAO() override;

		void Resize(uint32 width, uint32 height);

		virtual bool Init() override;
		virtual bool ConditionalRender(RenderContext& context) override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
		virtual void RenderUI(RenderContext& context) override;
	};
}

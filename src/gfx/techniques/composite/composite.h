#pragma once
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class Composite final : public RenderTechnique
	{
	public:
		Composite();

		virtual bool Init() override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
		virtual void RenderUI(RenderContext& context) override;
	};
}

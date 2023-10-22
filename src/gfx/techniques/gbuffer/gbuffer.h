#pragma once
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class GBuffer : public RenderTechnique
	{
	public:
		GBuffer();

		bool Init() override;
		void Render(RHI::CommandContext& cmd, RenderContext& context) override;
	};
}

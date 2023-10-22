#pragma once
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class DeferredLighting : public RenderTechnique
	{
	public:
		DeferredLighting();

		bool Init() override;
		void Render(RHI::CommandContext& cmd, RenderContext& context) override;
	};
}

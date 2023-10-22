#pragma once
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class Composite final : public RenderTechnique
	{
	private:
		struct RenderOptions
		{
			bool bEnableTonemap = true;
		} m_Options;

	public:
		Composite();

		void ConvertOptions(RenderContext& context);

		virtual OptionsList GetOptions() override;

		virtual bool Init() override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
	};
}

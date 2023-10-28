#include "stdafx.h"
#include "rendertechnique.h"

namespace limbo::Gfx
{
	RenderTechnique::RenderTechnique(const std::string_view& name)
		: m_Name(name)
	{
	}

	bool RenderTechnique::ConditionalRender(RenderContext& context)
	{
		return true;
	}

	void RenderTechnique::RenderUI(RenderContext& context)
	{
	}
}

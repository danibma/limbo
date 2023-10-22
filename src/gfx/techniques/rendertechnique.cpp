#include "stdafx.h"
#include "rendertechnique.h"

namespace limbo::Gfx
{
	RenderTechnique::RenderTechnique(const std::string_view& name)
		: m_Name(name)
	{
	}

	OptionsList RenderTechnique::GetOptions()
	{
		return {};
	}

	bool RenderTechnique::ConditionalRender(RenderContext& context)
	{
		return true;
	}
}

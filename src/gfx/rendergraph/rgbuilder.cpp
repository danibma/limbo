#include "stdafx.h"
#include "rgbuilder.h"

namespace limbo::Gfx
{
	RGPass& RGPass::DepthStencil(RGHandle texture)
	{
		ensure(EnumHasAllFlags(m_Flags, RGPassFlags::Graphics));
		m_DepthStencil = texture;
		return *this;
	}

	RGPass& RGPass::RenderTarget(RGHandle texture)
	{
		ensure(EnumHasAllFlags(m_Flags, RGPassFlags::Graphics));
		m_RenderTargets[m_NumRenderTargets] = texture;
		++m_NumRenderTargets;
		return *this;
	}

	void RGBuilder::Execute()
	{
		for (const RGPass& pass : m_Passes)
		{
			pass.m_Callback->Execute(m_Context);
		}
	}
}

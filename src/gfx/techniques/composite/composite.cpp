#include "stdafx.h"
#include "composite.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/device.h"

namespace limbo::Gfx
{
	Composite::Composite()
		: RenderTechnique("Scene Composite")
	{
	}

	void Composite::ConvertOptions(RenderContext& context)
	{
		RENDER_OPTION_GET(m_Options, bEnableTonemap, context.CurrentRenderOptions.Options);
	}

	OptionsList Composite::GetOptions()
	{
		OptionsList result;
		result.emplace(RENDER_OPTION_MAKE(m_Options, bEnableTonemap));
		return result;
	}

	bool Composite::Init()
	{
		return true;
	}

	void Composite::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		ConvertOptions(context);

		if (!ensure(context.SceneTextures.PreCompositeSceneTexture.IsValid()))
			context.SceneTextures.PreCompositeSceneTexture = RHI::ResourceManager::Ptr->EmptyTexture;

		cmd.BeginProfileEvent(m_Name.data());
		cmd.SetPipelineState(PSOCache::Get(PipelineID::Composite));
		cmd.SetPrimitiveTopology();
		cmd.SetRenderTargets(RHI::GetCurrentBackbuffer(), RHI::GetCurrentDepthBackbuffer());
		cmd.SetViewport(RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());

		cmd.ClearRenderTargets(RHI::GetCurrentBackbuffer());
		cmd.ClearDepthTarget(RHI::GetCurrentDepthBackbuffer());

		cmd.BindConstants(0, 0, m_Options.bEnableTonemap);

		cmd.BindConstants(0, 1, RM_GET(context.SceneTextures.PreCompositeSceneTexture)->SRV());

		cmd.Draw(6);
		cmd.EndProfileEvent(m_Name.data());
	}
}

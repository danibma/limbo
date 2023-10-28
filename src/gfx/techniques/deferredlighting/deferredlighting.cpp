#include "stdafx.h"
#include "deferredlighting.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	DeferredLighting::DeferredLighting()
		: RenderTechnique("Deferred Lighting")
	{
	}

	bool DeferredLighting::Init()
	{
		return true;
	}

	void DeferredLighting::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		cmd.BeginProfileEvent(m_Name.data());
		cmd.SetPipelineState(PSOCache::Get(PipelineID::PBRLighting));
		cmd.SetPrimitiveTopology();
		cmd.SetRenderTargets(context.SceneTextures.PreCompositeSceneTexture);
		cmd.SetViewport(context.RenderSize.x, context.RenderSize.y);

		cmd.BindTempConstantBuffer(1, context.SceneInfo);
		cmd.BindTempConstantBuffer(2, context.ShadowMapData);

		cmd.BindConstants(0, 0, context.IsAOEnabled());

		// PBR scene info
		cmd.BindConstants(0, 1, context.Light.Position);
		cmd.BindConstants(0, 4, context.Light.Color);


		// Bind deferred shading render targets
		cmd.BindConstants(3, 0, RM_GET(context.SceneTextures.GBufferRenderTargetA)->SRV());
		cmd.BindConstants(3, 1, RM_GET(context.SceneTextures.GBufferRenderTargetB)->SRV());
		cmd.BindConstants(3, 2, RM_GET(context.SceneTextures.GBufferRenderTargetC)->SRV());
		cmd.BindConstants(3, 3, RM_GET(context.SceneTextures.GBufferRenderTargetD)->SRV());
		cmd.BindConstants(3, 4, RM_GET(context.SceneTextures.GBufferRenderTargetE)->SRV());
		cmd.BindConstants(3, 5, RM_GET(context.SceneTextures.GBufferRenderTargetF)->SRV());

		cmd.BindConstants(3, 6, RM_GET(context.SceneTextures.AOTexture)->SRV());

		cmd.BindConstants(3, 7, RM_GET(context.SceneTextures.IrradianceMap)->SRV());
		cmd.BindConstants(3, 8, RM_GET(context.SceneTextures.PrefilterMap)->SRV());
		cmd.BindConstants(3, 9, RM_GET(context.SceneTextures.BRDFLUTMap)->SRV());

		cmd.Draw(6);
		cmd.EndProfileEvent(m_Name.data());
	}
}

#include "stdafx.h"
#include "gbuffer.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/scene.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	GBuffer::GBuffer()
		: RenderTechnique("GBuffer")
	{
	}

	bool GBuffer::Init()
	{
		return true;
	}

	void GBuffer::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		auto renderTargets = context.GetGBufferTextures();

		cmd.BeginProfileEvent(m_Name.data());
		cmd.SetPipelineState(PSOCache::Get(PipelineID::DeferredShading));
		cmd.SetPrimitiveTopology();
		cmd.SetRenderTargets(renderTargets, context.SceneTextures.GBufferDepthTarget);
		cmd.SetViewport(context.RenderSize.x, context.RenderSize.y);

		cmd.ClearRenderTargets(renderTargets);
		cmd.ClearDepthTarget(context.SceneTextures.GBufferDepthTarget, 0.0f);

		cmd.BindTempConstantBuffer(1, context.SceneInfo);
		for (const Scene* scene : context.GetScenes())
		{
			scene->IterateMeshes(TOnDrawMesh::CreateLambda([&](const Mesh& mesh)
			{
				cmd.BindConstants(0, 0, mesh.InstanceID);

				cmd.SetIndexBufferView(mesh.IndicesLocation);
				cmd.DrawIndexed((uint32)mesh.IndexCount);
			}));
		}
		cmd.EndProfileEvent(m_Name.data());
	}
}

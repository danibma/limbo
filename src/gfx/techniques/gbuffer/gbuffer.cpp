#include "stdafx.h"
#include "gbuffer.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/scene.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	namespace 
	{
		bool bMeshShadersRendering = true;
	}

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
		if (!bMeshShadersRendering)
			cmd.SetPipelineState(PSOCache::Get(PipelineID::DeferredShading));
		else
			cmd.SetPipelineState(PSOCache::Get(PipelineID::DeferredShading_Mesh));
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
				if (!bMeshShadersRendering)
					cmd.DrawIndexed((uint32)mesh.IndexCount);
				else
					cmd.DispatchMesh((uint32)mesh.MeshletsCount, 1, 1);
			}));
		}
		cmd.EndProfileEvent(m_Name.data());
	}

	void GBuffer::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("GBuffer"))
		{
			ImGui::Checkbox("Enable Mesh Shading", &bMeshShadersRendering);

			ImGui::TreePop();
		}
	}
}

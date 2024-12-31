#include "stdafx.h"
#include "skybox.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/scene.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	Skybox::Skybox()
		: RenderTechnique("Skybox")
	{
	}

	Skybox::~Skybox()
	{
		DestroyScene(m_SkyboxCube);
	}

	bool Skybox::Init()
	{
		m_SkyboxCube = Scene::Load("assets/models/skybox.glb");
		return true;
	}

	void Skybox::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		cmd.BeginProfileEvent(m_Name.data());
		cmd.SetPipelineState(PSOCache::Get(PipelineID::Skybox));
		cmd.SetPrimitiveTopology();
		cmd.SetRenderTargets(context.SceneTextures.PreCompositeSceneTexture);
		cmd.SetViewport(context.RenderSize.x, context.RenderSize.y);

		cmd.ClearRenderTargets(context.SceneTextures.PreCompositeSceneTexture);

		cmd.BindTempConstantBuffer(0, context.SceneInfo);

		cmd.BindConstants(1, 0, RM_GET(context.SceneTextures.EnvironmentCubemap)->SRV());

		m_SkyboxCube->IterateMeshes(TOnDrawMesh::CreateLambda([&](const Mesh& mesh)
		{
			cmd.SetVertexBufferView(mesh.VerticesLocation);
			cmd.SetIndexBufferView(mesh.IndicesLocation);
			cmd.DrawIndexed((uint32)mesh.IndexCount);
		}));
		cmd.EndProfileEvent(m_Name.data());
	}
}

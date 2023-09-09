#include "stdafx.h"
#include "shadowmapping.h"

#include "gfx/profiler.h"
#include "gfx/scene.h"
#include "gfx/scenerenderer.h"
#include "gfx/shaderinterop.h"
#include "gfx/uirenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	ShadowMapping::ShadowMapping()
	{
		m_ShadowMapShader = Gfx::CreateShader({
			.ProgramName = "shadowmap",
			.RTSize = { SHADOWMAP_SIZE, SHADOWMAP_SIZE },
			.RTFormats = {
				{
					.RTFormat = Gfx::Format::RGBA32_SFLOAT,
					.DebugName = "Shadow Map RT",
				}
			},
			.DepthFormat = {
				.RTFormat = Gfx::Format::D32_SFLOAT,
				.DebugName = "Shadow Map Depth",
			},
			.CullMode = D3D12_CULL_MODE_NONE,
			.Type = Gfx::ShaderType::Graphics
		});
	}

	ShadowMapping::~ShadowMapping()
	{
		DestroyShader(m_ShadowMapShader);
	}

	void ShadowMapping::BindShadowMap(Handle<Shader> shader)
	{
		SetParameter(shader, "g_ShadowMap", GetShaderDepthTarget(m_ShadowMapShader));
	}

	void ShadowMapping::Render(SceneRenderer* sceneRenderer)
	{
		if (UI::Globals::bDebugShadowMaps)
			DrawDebugWindow();

		// Shadow map
		BeginProfileEvent("Shadow Map");
		BindShader(m_ShadowMapShader);
		sceneRenderer->BindSceneInfo(m_ShadowMapShader);
		for (const Scene* scene : sceneRenderer->GetScenes())
		{
			scene->IterateMeshes([&](const Mesh& mesh)
			{
				SetParameter(m_ShadowMapShader, "instanceID", mesh.InstanceID);

				BindIndexBufferView(mesh.IndicesLocation);
				DrawIndexed((uint32)mesh.IndexCount);
			});
		}
		EndProfileEvent("Shadow Map");
}

	void ShadowMapping::DrawDebugWindow()
	{
		ImGui::Begin("Shadow Map Debug", &UI::Globals::bDebugShadowMaps);
		ImGui::Image((ImTextureID)Gfx::GetShaderRTTextureID(m_ShadowMapShader, 0), ImVec2(512, 512));
		ImGui::End();
	}
}

#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	RTAO::RTAO()
	{
		m_FinalTexture = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "RTAO Final Texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Format::RGBA8_UNORM,
			.Type = TextureType::Texture2D,
		});

		m_RTAOShader = CreateShader({
			.ProgramName = "RTAO",
			.Libs = {
				{
					.LibName = "raytracing/rtao",
					.HitGroupsDescriptions = 
					{
						{
							.HitGroupExport = L"RTAOHitGroup",
							.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
							.AnyHitShaderImport = L"RTAOAnyHit",
						}
					},
					.Exports = 
					{
						{ .Name = L"RTAORayGen" },
						{ .Name = L"RTAOAnyHit" },
						{ .Name = L"RTAOMiss" }
					},
					
				},
			},
			.ShaderConfig = {
				.MaxPayloadSizeInBytes = sizeof(float), // AOPayload
				.MaxAttributeSizeInBytes = sizeof(float2) // float2 barycentrics
			},
			.Type = ShaderType::RayTracing,
		});
	}

	RTAO::~RTAO()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_RTAOShader.IsValid())
			DestroyShader(m_RTAOShader);
	}

	void RTAO::Render(SceneRenderer* sceneRenderer, AccelerationStructure* sceneAS, Handle<Texture> positionsMap, Handle<Texture> normalsMap)
	{
		BeginEvent("RTAO");
		BindShader(m_RTAOShader);

		ShaderBindingTable SBT(m_RTAOShader);
		SBT.BindRayGen(L"RTAORayGen");
		SBT.BindMissShader(L"RTAOMiss");
		SBT.BindHitGroup(L"RTAOHitGroup");

		sceneRenderer->BindSceneInfo(m_RTAOShader);
		SetParameter(m_RTAOShader, "SceneAS", sceneAS);
		SetParameter(m_RTAOShader, "radius", sceneRenderer->Tweaks.SSAORadius);
		SetParameter(m_RTAOShader, "g_Positions", positionsMap);
		SetParameter(m_RTAOShader, "g_Normals", normalsMap);
		SetParameter(m_RTAOShader, "g_Output", m_FinalTexture);
		SetParameter(m_RTAOShader, "LinearWrap", GetDefaultLinearWrapSampler());
		DispatchRays(SBT, GetBackbufferWidth(), GetBackbufferHeight());
		EndEvent();
	}

	Handle<Texture> RTAO::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}

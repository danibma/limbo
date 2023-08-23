#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	RTAO::RTAO()
	{
		m_NoisedTexture = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "RTAO Texture W/ Noise",
			.Flags = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource,
			.Format = Format::RGBA8_UNORM,
			.Type = TextureType::Texture2D,
		});

		PreparePreviousFrameTexture();

		m_FinalTexture = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "RTAO Final Texture",
			.Flags = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource,
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

		m_DenoiseRTAOShader = Gfx::CreateShader({
			.ProgramName = "raytracing/rtaoaccumulate",
			.CsEntryPoint = "RTAOAccumulate",
			.Type = Gfx::ShaderType::Compute
		});
	}

	RTAO::~RTAO()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_RTAOShader.IsValid())
			DestroyShader(m_RTAOShader);
		if (m_DenoiseRTAOShader.IsValid())
			DestroyShader(m_DenoiseRTAOShader);
		if (m_NoisedTexture.IsValid())
			DestroyTexture(m_NoisedTexture);
		if (m_PreviousFrame.IsValid())
			DestroyTexture(m_PreviousFrame);
	}

	void RTAO::Render(SceneRenderer* sceneRenderer, AccelerationStructure* sceneAS, Handle<Texture> positionsMap, Handle<Texture> normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		BeginEvent("RTAO");
		BindShader(m_RTAOShader);

		ShaderBindingTable SBT(m_RTAOShader);
		SBT.BindRayGen(L"RTAORayGen");
		SBT.BindMissShader(L"RTAOMiss");
		SBT.BindHitGroup(L"RTAOHitGroup");

		sceneRenderer->BindSceneInfo(m_RTAOShader);
		SetParameter(m_RTAOShader, "SceneAS", sceneAS);
		SetParameter(m_RTAOShader, "radius", sceneRenderer->Tweaks.SSAORadius);
		SetParameter(m_RTAOShader, "power", sceneRenderer->Tweaks.SSAOPower);
		SetParameter(m_RTAOShader, "samples", sceneRenderer->Tweaks.RTAOSamples);
		SetParameter(m_RTAOShader, "g_Positions", positionsMap);
		SetParameter(m_RTAOShader, "g_Normals", normalsMap);
		SetParameter(m_RTAOShader, "g_Output", m_NoisedTexture);
		DispatchRays(SBT, GetBackbufferWidth(), GetBackbufferHeight());
		EndEvent();

		BeginEvent("RTAO Denoise");
		BindShader(m_DenoiseRTAOShader);
		SetParameter(m_DenoiseRTAOShader, "accumCount", m_AccumCount);
		SetParameter(m_DenoiseRTAOShader, "g_PreviousRTAOImage", m_PreviousFrame);
		SetParameter(m_DenoiseRTAOShader, "g_CurrentRTAOImage", m_NoisedTexture);
		SetParameter(m_DenoiseRTAOShader, "g_DenoisedRTAOImage", m_FinalTexture);
		Dispatch(GetBackbufferWidth() / 8, GetBackbufferHeight() / 8, 1);
		EndEvent();

		++m_AccumCount;

		Device::Ptr->GetCommandContext(ContextType::Direct)->UAVBarrier(m_FinalTexture);
		CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);
	}

	Handle<Texture> RTAO::GetFinalTexture() const
	{
		return m_FinalTexture;
	}

	void RTAO::PreparePreviousFrameTexture()
	{
		m_AccumCount = 0;
		m_PreviousFrame = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "RTAO Previous Frame",
			.Flags = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource,
			.Format = Format::RGBA8_UNORM,
			.Type = TextureType::Texture2D,
		});
	}
}

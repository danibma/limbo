#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	RTAO::RTAO()
	{
		m_NoisedTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Texture W/ Noise",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});

		PreparePreviousFrameTexture();

		m_FinalTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Final Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});

		m_RTAOShader = RHI::CreateShader({
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
			.Type = RHI::ShaderType::RayTracing,
		});

		m_DenoiseRTAOShader = RHI::CreateShader({
			.ProgramName = "raytracing/rtaoaccumulate",
			.CsEntryPoint = "RTAOAccumulate",
			.Type = RHI::ShaderType::Compute
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

	void RTAO::Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		RHI::BeginProfileEvent("RTAO");
		RHI::BindShader(m_RTAOShader);

		RHI::ShaderBindingTable SBT(m_RTAOShader);
		SBT.BindRayGen(L"RTAORayGen");
		SBT.BindMissShader(L"RTAOMiss");
		SBT.BindHitGroup(L"RTAOHitGroup");

		sceneRenderer->BindSceneInfo(m_RTAOShader);
		RHI::SetParameter(m_RTAOShader, "SceneAS", sceneAS);
		RHI::SetParameter(m_RTAOShader, "radius", sceneRenderer->Tweaks.SSAORadius);
		RHI::SetParameter(m_RTAOShader, "power", sceneRenderer->Tweaks.SSAOPower);
		RHI::SetParameter(m_RTAOShader, "samples", sceneRenderer->Tweaks.RTAOSamples);
		RHI::SetParameter(m_RTAOShader, "g_Positions", positionsMap);
		RHI::SetParameter(m_RTAOShader, "g_Normals", normalsMap);
		RHI::SetParameter(m_RTAOShader, "g_Output", m_NoisedTexture);
		RHI::DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		RHI::EndProfileEvent("RTAO");

		RHI::BeginProfileEvent("RTAO Denoise");
		RHI::BindShader(m_DenoiseRTAOShader);
		RHI::SetParameter(m_DenoiseRTAOShader, "accumCount", m_AccumCount);
		RHI::SetParameter(m_DenoiseRTAOShader, "g_PreviousRTAOImage", m_PreviousFrame);
		RHI::SetParameter(m_DenoiseRTAOShader, "g_CurrentRTAOImage", m_NoisedTexture);
		RHI::SetParameter(m_DenoiseRTAOShader, "g_DenoisedRTAOImage", m_FinalTexture);
		RHI::Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
		RHI::EndProfileEvent("RTAO Denoise");

		++m_AccumCount;

		RHI::GetCommandContext()->InsertUAVBarrier(m_FinalTexture);
		RHI::CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);
	}

	RHI::Handle<RHI::Texture> RTAO::GetFinalTexture() const
	{
		return m_FinalTexture;
	}

	void RTAO::PreparePreviousFrameTexture()
	{
		m_AccumCount = 0;
		m_PreviousFrame = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Previous Frame",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});
	}
}

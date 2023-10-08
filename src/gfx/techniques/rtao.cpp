#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/rootsignature.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/shadercompiler.h"

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

		m_CommonRS = new RHI::RootSignature("RTAO Common RS");
		m_CommonRS->AddRootSRV(0);
		m_CommonRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
		m_CommonRS->AddRootCBV(100);
		m_CommonRS->AddRootConstants(0, 5);
		m_CommonRS->Create();

		m_RTAOShader = RHI::CreateShader("raytracing/rtao.hlsl", "", RHI::ShaderType::Lib);
		RHI::SC::Compile(m_RTAOShader);
		{
			RHI::RaytracingLibDesc libDesc = {};
			libDesc.AddExport(L"RTAORayGen");
			libDesc.AddExport(L"RTAOAnyHit");
			libDesc.AddExport(L"RTAOMiss");
			libDesc.AddHitGroup(L"RTAOHitGroup", L"RTAOAnyHit");

			RHI::RaytracingPipelineStateInitializer psoInit = {};
			psoInit.SetGlobalRootSignature(m_CommonRS);
			psoInit.AddLib(m_RTAOShader, libDesc);
			psoInit.SetShaderConfig(sizeof(float) /* AOPayload */, sizeof(float2) /* BuiltInTriangleIntersectionAttributes */);
			psoInit.SetName("RTAO PSO");
			m_RTAOPSO = RHI::CreatePSO(psoInit);
		}

		m_DenoiseRTAOShader = RHI::CreateShader("raytracing/rtaoaccumulate.hlsl", "RTAOAccumulate", RHI::ShaderType::Compute);
		RHI::SC::Compile(m_DenoiseRTAOShader);
		{
			RHI::PipelineStateInitializer psoInit = {};
			psoInit.SetRootSignature(m_CommonRS);
			psoInit.SetComputeShader(m_DenoiseRTAOShader);
			psoInit.SetName("RTAO Accumulate PSO");
			m_RTAODenoisePSO = RHI::CreatePSO(psoInit);
		}
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

		RHI::DestroyPSO(m_RTAOPSO);
		RHI::DestroyPSO(m_RTAODenoisePSO);
		delete m_CommonRS;
	}

	void RTAO::Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::TextureHandle positionsMap, RHI::TextureHandle normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		{
			RHI::BeginProfileEvent("RTAO");
			RHI::SetPipelineState(m_RTAOPSO);

			RHI::ShaderBindingTable SBT(m_RTAOPSO);
			SBT.BindRayGen(L"RTAORayGen");
			SBT.BindMissShader(L"RTAOMiss");
			SBT.BindHitGroup(L"RTAOHitGroup");

			RHI::BindRootSRV(0, sceneAS->GetTLASBuffer()->Resource->GetGPUVirtualAddress());

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_NoisedTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(1, uavHandles, 1);

			RHI::BindTempConstantBuffer(2, sceneRenderer->SceneInfo);

			RHI::BindConstants(3, 0, sceneRenderer->Tweaks.SSAORadius);
			RHI::BindConstants(3, 1, sceneRenderer->Tweaks.SSAOPower);
			RHI::BindConstants(3, 2, sceneRenderer->Tweaks.RTAOSamples);
			RHI::BindConstants(3, 3, RM_GET(positionsMap)->SRV());
			RHI::BindConstants(3, 4, RM_GET(normalsMap)->SRV());
			RHI::DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
			RHI::EndProfileEvent("RTAO");
		}

		{
			RHI::BeginProfileEvent("RTAO Denoise");
			RHI::SetPipelineState(m_RTAODenoisePSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_FinalTexture)->UAVHandle[0],
			};
			RHI::BindTempDescriptorTable(1, uavHandles, _countof(uavHandles));

			RHI::BindConstants(3, 0, m_AccumCount);
			RHI::BindConstants(3, 1, RM_GET(m_PreviousFrame)->SRV());
			RHI::BindConstants(3, 2, RM_GET(m_NoisedTexture)->SRV());
			RHI::Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
			RHI::EndProfileEvent("RTAO Denoise");
		}

		++m_AccumCount;

		RHI::GetCommandContext()->InsertUAVBarrier(m_FinalTexture);
		RHI::CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);
	}

	RHI::TextureHandle RTAO::GetFinalTexture() const
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

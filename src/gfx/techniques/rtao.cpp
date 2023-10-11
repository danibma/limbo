#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/rootsignature.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/shadercompiler.h"
#include "gfx/rhi/commandcontext.h"

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

		m_CommonRS = RHI::CreateRootSignature("RTAO Common RS", RHI::RSSpec().Init().AddRootSRV(0).AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootCBV(100).AddRootConstants(0, 5));

		m_RTAOShader = RHI::CreateShader("raytracing/rtao.hlsl", "", RHI::ShaderType::Lib);
		RHI::SC::Compile(m_RTAOShader);
		{
			RHI::RTLibSpec libDesc = RHI::RTLibSpec()
				.Init()
				.AddExport(L"RTAORayGen")
				.AddExport(L"RTAOAnyHit")
				.AddExport(L"RTAOMiss")
				.AddHitGroup(L"RTAOHitGroup", L"RTAOAnyHit");

			RHI::RTPipelineStateSpec psoInit = RHI::RTPipelineStateSpec()
				.Init()
				.SetGlobalRootSignature(m_CommonRS)
				.AddLib(m_RTAOShader, libDesc)
				.SetShaderConfig(sizeof(float) /* AOPayload */, sizeof(float2) /* BuiltInTriangleIntersectionAttributes */)
				.SetName("RTAO PSO");
			m_RTAOPSO = RHI::CreatePSO(psoInit);
		}

		m_DenoiseRTAOShader = RHI::CreateShader("raytracing/rtaoaccumulate.hlsl", "RTAOAccumulate", RHI::ShaderType::Compute);
		RHI::SC::Compile(m_DenoiseRTAOShader);
		{
			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec()
				.Init()
                .SetRootSignature(m_CommonRS)
                .SetComputeShader(m_DenoiseRTAOShader)
                .SetName("RTAO Accumulate PSO");
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
		RHI::DestroyRootSignature(m_CommonRS);
	}

	void RTAO::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::TextureHandle positionsMap, RHI::TextureHandle normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		{
			cmd->BeginProfileEvent("RTAO");
			cmd->SetPipelineState(m_RTAOPSO);

			RHI::ShaderBindingTable SBT(m_RTAOPSO);
			SBT.BindRayGen(L"RTAORayGen");
			SBT.BindMissShader(L"RTAOMiss");
			SBT.BindHitGroup(L"RTAOHitGroup");

			cmd->BindRootSRV(0, sceneAS->GetTLASBuffer()->Resource->GetGPUVirtualAddress());

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_NoisedTexture)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(1, uavHandles, 1);

			cmd->BindTempConstantBuffer(2, sceneRenderer->SceneInfo);

			cmd->BindConstants(3, 0, sceneRenderer->Tweaks.SSAORadius);
			cmd->BindConstants(3, 1, sceneRenderer->Tweaks.SSAOPower);
			cmd->BindConstants(3, 2, sceneRenderer->Tweaks.RTAOSamples);
			cmd->BindConstants(3, 3, RM_GET(positionsMap)->SRV());
			cmd->BindConstants(3, 4, RM_GET(normalsMap)->SRV());
			cmd->DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
			cmd->EndProfileEvent("RTAO");
		}

		{
			cmd->BeginProfileEvent("RTAO Denoise");
			cmd->SetPipelineState(m_RTAODenoisePSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_FinalTexture)->UAVHandle[0],
			};
			cmd->BindTempDescriptorTable(1, uavHandles, _countof(uavHandles));

			cmd->BindConstants(3, 0, m_AccumCount);
			cmd->BindConstants(3, 1, RM_GET(m_PreviousFrame)->SRV());
			cmd->BindConstants(3, 2, RM_GET(m_NoisedTexture)->SRV());
			cmd->Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
			cmd->EndProfileEvent("RTAO Denoise");
		}

		++m_AccumCount;

		cmd->InsertUAVBarrier(m_FinalTexture);
		cmd->CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);
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

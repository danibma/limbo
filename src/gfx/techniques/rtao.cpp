#include "stdafx.h"
#include "rtao.h"

#include "gfx/psocache.h"
#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
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
	}

	RTAO::~RTAO()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_NoisedTexture.IsValid())
			DestroyTexture(m_NoisedTexture);
		if (m_PreviousFrame.IsValid())
			DestroyTexture(m_PreviousFrame);
	}

	void RTAO::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::TextureHandle positionsMap, RHI::TextureHandle normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		{
			RHI::PSOHandle pso = PSOCache::Get(PipelineID::RTAO);

			cmd->BeginProfileEvent("RTAO");
			cmd->SetPipelineState(pso);

			RHI::ShaderBindingTable SBT(pso);
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
			cmd->SetPipelineState(PSOCache::Get(PipelineID::RTAOAccumulate));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_FinalTexture)->UAVHandle[0],
			};
			cmd->BindTempDescriptorTable(1, uavHandles, ARRAY_LEN(uavHandles));

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

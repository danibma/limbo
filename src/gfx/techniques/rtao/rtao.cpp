#include "stdafx.h"
#include "rtao.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shadercompiler.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	namespace
	{
		int	  s_RTAOSamples = 2;
		float s_RTAORadius = 0.3f;
		float s_RTAOPower = 1.2f;
	}

	RTAO::RTAO()
		: RenderTechnique("RTAO")
	{
	}

	RTAO::~RTAO()
	{
		if (m_NoisedTexture.IsValid())
			DestroyTexture(m_NoisedTexture);
		if (m_PreviousFrame.IsValid())
			DestroyTexture(m_PreviousFrame);
	}

	bool RTAO::Init()
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

		return true;
	}

	bool RTAO::ConditionalRender(RenderContext& context)
	{
		return context.CanRenderRTAO();
	}

	void RTAO::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		if (context.SceneInfo.PrevView != context.SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		{
			RHI::PSOHandle pso = PSOCache::Get(PipelineID::RTAO);

			cmd.BeginProfileEvent("RTAO");
			cmd.SetPipelineState(pso);

			RHI::ShaderBindingTable SBT(pso);
			SBT.BindRayGen(L"RTAORayGen");
			SBT.BindMissShader(L"RTAOMiss");
			SBT.BindHitGroup(L"RTAOHitGroup");

			cmd.BindRootSRV(0, context.SceneAccelerationStructure.GetTLASBuffer()->Resource->GetGPUVirtualAddress());

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_NoisedTexture)->UAVHandle[0]
			};
			cmd.BindTempDescriptorTable(1, uavHandles, 1);

			cmd.BindTempConstantBuffer(2, context.SceneInfo);

			cmd.BindConstants(3, 0, s_RTAORadius);
			cmd.BindConstants(3, 1, s_RTAOPower);
			cmd.BindConstants(3, 2, s_RTAOSamples);
			cmd.BindConstants(3, 3, RM_GET(context.SceneTextures.GBufferRenderTargetB)->SRV());
			cmd.BindConstants(3, 4, RM_GET(context.SceneTextures.GBufferRenderTargetD)->SRV());
			cmd.DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
			cmd.EndProfileEvent("RTAO");
		}

		{
			cmd.BeginProfileEvent("RTAO Denoise");
			cmd.SetPipelineState(PSOCache::Get(PipelineID::RTAOAccumulate));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(context.SceneTextures.AOTexture)->UAVHandle[0],
			};
			cmd.BindTempDescriptorTable(1, uavHandles, ARRAY_LEN(uavHandles));

			cmd.BindConstants(3, 0, m_AccumCount);
			cmd.BindConstants(3, 1, RM_GET(m_PreviousFrame)->SRV());
			cmd.BindConstants(3, 2, RM_GET(m_NoisedTexture)->SRV());
			cmd.Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
			cmd.EndProfileEvent("RTAO Denoise");
		}

		++m_AccumCount;

		cmd.InsertUAVBarrier(context.SceneTextures.AOTexture);
		cmd.CopyTextureToTexture(context.SceneTextures.AOTexture, m_PreviousFrame);
	}

	void RTAO::RenderUI(RenderContext& context)
	{
		ImGui::DragFloat("RTAO Radius", &s_RTAORadius, 0.1f, 0.0f, 1.0f);
		ImGui::DragFloat("RTAO Power", &s_RTAOPower, 0.1f, 0.0f, 2.0f);
		ImGui::DragInt("RTAO Samples", &s_RTAOSamples, 1, 0, 16);
	}

	void RTAO::PreparePreviousFrameTexture()
	{
		m_AccumCount = 0;
		m_PreviousFrame = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Previous Frame",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});
	}
}

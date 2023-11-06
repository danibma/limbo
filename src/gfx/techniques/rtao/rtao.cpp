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
		RHI::DestroyTexture(m_NoisedTexture);
		RHI::DestroyTexture(m_PreviousFrame);
		RHI::DestroyTexture(m_FinalTexture);
	}

	void RTAO::OnResize(uint32 width, uint32 height)
	{
		if (m_NoisedTexture.IsValid())
			RHI::DestroyTexture(m_NoisedTexture);
		if (m_FinalTexture.IsValid())
			RHI::DestroyTexture(m_FinalTexture);
		if (m_PreviousFrame.IsValid())
			RHI::DestroyTexture(m_PreviousFrame);

		m_NoisedTexture = RHI::CreateTexture({
			.Width = width,
			.Height = height,
			.DebugName = "RTAO Texture W/ Noise",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});

		m_FinalTexture = RHI::CreateTexture({
			.Width = width,
			.Height = height,
			.DebugName = "AO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});

		PreparePreviousFrameTexture();
	}

	bool RTAO::Init()
	{
		OnResize(RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());

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
				RM_GET(m_FinalTexture)->UAVHandle[0],
			};
			cmd.BindTempDescriptorTable(1, uavHandles, ARRAY_LEN(uavHandles));

			cmd.BindConstants(3, 0, m_AccumCount);
			cmd.BindConstants(3, 1, RM_GET(m_PreviousFrame)->SRV());
			cmd.BindConstants(3, 2, RM_GET(m_NoisedTexture)->SRV());
			cmd.Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
			cmd.EndProfileEvent("RTAO Denoise");
		}

		++m_AccumCount;

		cmd.InsertUAVBarrier(m_FinalTexture);
		cmd.CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);

		context.SceneTextures.AOTexture = m_FinalTexture;
	}

	void RTAO::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("AO"))
		{
			ImGui::DragFloat("Radius", &s_RTAORadius, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("Power", &s_RTAOPower, 0.1f, 0.0f, 2.0f);
			ImGui::DragInt("Samples", &s_RTAOSamples, 1, 0, 16);

			ImGui::TreePop();
		}
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

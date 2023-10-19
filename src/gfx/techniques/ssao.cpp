#include "stdafx.h"
#include "ssao.h"

#include "gfx/psocache.h"
#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shadercompiler.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	SSAO::SSAO()
	{
		m_UnblurredSSAOTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "SSAO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});

		m_BlurredSSAOTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "Blurred SSAO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_BlurredSSAOTexture);
	}

	void SSAO::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::TextureHandle positionsMap, RHI::TextureHandle sceneDepthMap)
	{
		{
			cmd->BeginProfileEvent("SSAO");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::SSAO));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_UnblurredSSAOTexture)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(0, uavHandles, ARRAY_LEN(uavHandles));

			cmd->BindConstants(1, 0, sceneRenderer->Tweaks.SSAORadius);
			cmd->BindConstants(1, 1, sceneRenderer->Tweaks.SSAOPower);
			cmd->BindConstants(1, 2, RM_GET(positionsMap)->SRV());
			cmd->BindConstants(1, 3, RM_GET(sceneDepthMap)->SRV());

			cmd->BindTempConstantBuffer(2, sceneRenderer->SceneInfo);

			cmd->Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			cmd->EndProfileEvent("SSAO");
		}

		{
			cmd->BeginProfileEvent("SSAO Blur Texture");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::SSAOBlur));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_BlurredSSAOTexture)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(0, uavHandles, ARRAY_LEN(uavHandles));

			cmd->BindConstants(1, 4, RM_GET(m_UnblurredSSAOTexture)->SRV());

			cmd->Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			cmd->EndProfileEvent("SSAO Blur Texture");
		}
	}

	RHI::TextureHandle SSAO::GetBlurredTexture() const
	{
		return m_BlurredSSAOTexture;
	}
}

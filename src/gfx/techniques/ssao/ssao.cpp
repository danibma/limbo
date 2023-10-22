#include "stdafx.h"
#include "ssao.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shadercompiler.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	SSAO::SSAO()
		: RenderTechnique("SSAO")
	{
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
	}

	bool SSAO::Init()
	{
		m_UnblurredSSAOTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "SSAO Unblurred Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});

		return true;
	}

	bool SSAO::ConditionalRender(RenderContext& context)
	{
		return context.Tweaks.CurrentAOTechnique == (int)AmbientOcclusion::SSAO;
	}

	void SSAO::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		{
			cmd.BeginProfileEvent("SSAO");
			cmd.SetPipelineState(PSOCache::Get(PipelineID::SSAO));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_UnblurredSSAOTexture)->UAVHandle[0]
			};
			cmd.BindTempDescriptorTable(0, uavHandles, ARRAY_LEN(uavHandles));

			cmd.BindConstants(1, 0, context.Tweaks.SSAORadius);
			cmd.BindConstants(1, 1, context.Tweaks.SSAOPower);
			cmd.BindConstants(1, 2, RM_GET(context.SceneTextures.GBufferRenderTargetA)->SRV());
			cmd.BindConstants(1, 3, RM_GET(context.SceneTextures.GBufferDepthTarget)->SRV());

			cmd.BindTempConstantBuffer(2, context.SceneInfo);

			cmd.Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			cmd.EndProfileEvent("SSAO");
		}

		{
			cmd.BeginProfileEvent("SSAO Blur Texture");
			cmd.SetPipelineState(PSOCache::Get(PipelineID::SSAOBlur));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(context.SceneTextures.AOTexture)->UAVHandle[0]
			};
			cmd.BindTempDescriptorTable(0, uavHandles, ARRAY_LEN(uavHandles));

			cmd.BindConstants(1, 4, RM_GET(m_UnblurredSSAOTexture)->SRV());

			cmd.Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			cmd.EndProfileEvent("SSAO Blur Texture");
		}
	}
}

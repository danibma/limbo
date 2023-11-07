#include "stdafx.h"
#include "ssao.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shadercompiler.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/techniques/helpers/blur.h"

namespace limbo::Gfx
{
	namespace
	{
		float s_SSAORadius = 0.3f;
		float s_SSAOPower  = 1.2f;

		int s_BlurType = 0; // 0 = 4x4 Box Blur, 1 = 3x3 Gaussian Blur
		int s_SSAORes = 1;

		uint32 s_SSAOResWidth;
		uint32 s_SSAOResHeight;
	}

	SSAO::SSAO()
		: RenderTechnique("SSAO")
	{
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_FinalSSAOTexture);
	}

	void SSAO::OnResize(uint32 width, uint32 height)
	{
		if (m_FinalSSAOTexture.IsValid())
			RHI::DestroyTexture(m_FinalSSAOTexture);
		if (m_UnblurredSSAOTexture.IsValid())
			RHI::DestroyTexture(m_UnblurredSSAOTexture);

		s_SSAOResWidth = width >> s_SSAORes;
		s_SSAOResHeight = height >> s_SSAORes;

		m_FinalSSAOTexture = RHI::CreateTexture({
			.Width = s_SSAOResWidth,
			.Height = s_SSAOResHeight,
			.DebugName = "AO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});

		m_UnblurredSSAOTexture = RHI::CreateTexture({
			.Width = s_SSAOResWidth,
			.Height = s_SSAOResHeight,
			.DebugName = "SSAO Unblurred Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});
	}

	bool SSAO::Init()
	{
		OnResize(RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		return true;
	}

	bool SSAO::ConditionalRender(RenderContext& context)
	{
		return context.CanRenderSSAO();
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

			cmd.BindConstants(1, 0, s_SSAORadius);
			cmd.BindConstants(1, 1, s_SSAOPower);
			cmd.BindConstants(1, 2, RM_GET(context.SceneTextures.GBufferRenderTargetA)->SRV());
			cmd.BindConstants(1, 3, RM_GET(context.SceneTextures.GBufferDepthTarget)->SRV());

			cmd.BindTempConstantBuffer(2, context.SceneInfo);

			cmd.Dispatch(Math::DivideAndRoundUp(s_SSAOResWidth, 16u), Math::DivideAndRoundUp(s_SSAOResHeight, 16u), 1);
			cmd.EndProfileEvent("SSAO");
		}

		if (s_BlurType == 0)
		{
			cmd.BeginProfileEvent("SSAO Box Blur");
			cmd.SetPipelineState(PSOCache::Get(PipelineID::SSAO_BoxBlur));

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_FinalSSAOTexture)->UAVHandle[0]
			};
			cmd.BindTempDescriptorTable(0, uavHandles, ARRAY_LEN(uavHandles));

			cmd.BindConstants(1, 4, RM_GET(m_UnblurredSSAOTexture)->SRV());

			cmd.Dispatch(Math::DivideAndRoundUp(s_SSAOResWidth, 16u), Math::DivideAndRoundUp(s_SSAOResHeight, 16u), 1);
			cmd.EndProfileEvent("SSAO Box Blur");
		}
		else if (s_BlurType == 1)
		{
			TechniqueHelpers::BlurPass::Execute(cmd, m_UnblurredSSAOTexture, m_FinalSSAOTexture, { s_SSAOResWidth, s_SSAOResHeight }, "SSAO");
		}

		context.SceneTextures.AOTexture = m_FinalSSAOTexture;
	}

	void SSAO::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("AO"))
		{
			ImGui::DragFloat("Radius", &s_SSAORadius, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("Power", &s_SSAOPower, 0.1f, 0.0f, 2.0f);

			const char* resStrings = "Full\0Half\0";
			int currentSSAORes = s_SSAORes;
			if (ImGui::Combo("Resolution", &currentSSAORes, resStrings))
			{
				if (currentSSAORes != s_SSAORes)
				{
					s_SSAORes = currentSSAORes;
					OnResize(RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
				}
			}

			const char* blurStrings = "4x4 Box\0 9-tap Gaussian\0";
			int currentSSAOBlur = s_BlurType;
			if (ImGui::Combo("Blur Type", &currentSSAOBlur, blurStrings))
			{
				if (currentSSAOBlur != s_BlurType)
				{
					s_BlurType = currentSSAOBlur;
				}
			}
			

			ImGui::TreePop();
		}
	}
}

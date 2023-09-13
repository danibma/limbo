#include "stdafx.h"
#include "ssao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

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
		m_SSAOShader = RHI::CreateShader({
			.ProgramName = "ssao",
			.CsEntryPoint = "ComputeSSAO",
			.Type = RHI::ShaderType::Compute
		});

		m_BlurredSSAOTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "Blurred SSAO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});
		m_BlurSSAOShader = RHI::CreateShader({
			.ProgramName = "ssao",
			.CsEntryPoint = "BlurSSAO",
			.Type = RHI::ShaderType::Compute
		});
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_BlurredSSAOTexture);
		RHI::DestroyShader(m_SSAOShader);
		RHI::DestroyShader(m_BlurSSAOShader);
	}

	void SSAO::Render(SceneRenderer* sceneRenderer, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> sceneDepthMap)
	{
		RHI::BeginProfileEvent("SSAO");
		RHI::BindShader(m_SSAOShader);
		sceneRenderer->BindSceneInfo(m_SSAOShader);
		RHI::SetParameter(m_SSAOShader, "g_Positions", positionsMap);
		RHI::SetParameter(m_SSAOShader, "g_UnblurredSSAOTexture", m_UnblurredSSAOTexture);
		RHI::SetParameter(m_SSAOShader, "g_SceneDepth", sceneDepthMap);
		RHI::SetParameter(m_SSAOShader, "radius", sceneRenderer->Tweaks.SSAORadius);
		RHI::SetParameter(m_SSAOShader, "power", sceneRenderer->Tweaks.SSAOPower);
		RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
		RHI::EndProfileEvent("SSAO");

		RHI::BeginProfileEvent("SSAO Blur Texture");
		RHI::BindShader(m_BlurSSAOShader);
		RHI::SetParameter(m_BlurSSAOShader, "g_SSAOTexture", m_UnblurredSSAOTexture);
		RHI::SetParameter(m_BlurSSAOShader, "g_BlurredSSAOTexture", m_BlurredSSAOTexture);
		RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
		RHI::EndProfileEvent("SSAO Blur Texture");
	}

	RHI::Handle<RHI::Texture> SSAO::GetBlurredTexture() const
	{
		return m_BlurredSSAOTexture;
	}
}

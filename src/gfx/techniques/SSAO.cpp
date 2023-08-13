﻿#include "stdafx.h"
#include "SSAO.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	SSAO::SSAO()
	{
		m_UnblurredSSAOTexture = Gfx::CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "SSAO Texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Gfx::Format::R8_UNORM,
		});
		m_SSAOShader = Gfx::CreateShader({
			.ProgramName = "ssao",
			.CsEntryPoint = "ComputeSSAO",
			.Type = Gfx::ShaderType::Compute
		});

		m_BlurredSSAOTexture = Gfx::CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "Blurred SSAO Texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Gfx::Format::R8_UNORM,
		});
		m_BlurSSAOShader = Gfx::CreateShader({
			.ProgramName = "ssao",
			.CsEntryPoint = "BlurSSAO",
			.Type = Gfx::ShaderType::Compute
		});
	}

	SSAO::~SSAO()
	{
		DestroyTexture(m_UnblurredSSAOTexture);
		DestroyTexture(m_BlurredSSAOTexture);
		DestroyShader(m_SSAOShader);
		DestroyShader(m_BlurSSAOShader);
	}

	void SSAO::Render(SceneRenderer* sceneRenderer, Handle<Texture> positionsMap, Handle<Texture> sceneDepthMap)
	{
		BeginEvent("SSAO");
		BindShader(m_SSAOShader);
		SetParameter(m_SSAOShader, "g_Positions", positionsMap);
		SetParameter(m_SSAOShader, "g_UnblurredSSAOTexture", m_UnblurredSSAOTexture);
		SetParameter(m_SSAOShader, "g_SceneDepth", sceneDepthMap);
		SetParameter(m_SSAOShader, "LinearWrap", GetDefaultLinearWrapSampler());
		SetParameter(m_SSAOShader, "PointClamp", GetDefaultPointClampSampler());
		SetParameter(m_SSAOShader, "radius", sceneRenderer->Tweaks.SSAORadius);
		SetParameter(m_SSAOShader, "power", sceneRenderer->Tweaks.SSAOPower);
		SetParameter(m_SSAOShader, "projection", sceneRenderer->Camera.Proj);
		SetParameter(m_SSAOShader, "invProjection", glm::inverse(sceneRenderer->Camera.Proj));
		SetParameter(m_SSAOShader, "frameIndex", sceneRenderer->FrameIndex);
		Dispatch(GetBackbufferWidth() / 16, GetBackbufferHeight() / 16, 1);
		EndEvent();

		BeginEvent("SSAO Blur Texture");
		BindShader(m_BlurSSAOShader);
		SetParameter(m_BlurSSAOShader, "LinearWrap", GetDefaultLinearWrapSampler());
		SetParameter(m_BlurSSAOShader, "g_SSAOTexture", m_UnblurredSSAOTexture);
		SetParameter(m_BlurSSAOShader, "g_BlurredSSAOTexture", m_BlurredSSAOTexture);
		Dispatch(GetBackbufferWidth() / 16, GetBackbufferHeight() / 16, 1);
		EndEvent();
	}

	Handle<Texture> SSAO::GetBlurredTexture() const
	{
		return m_BlurredSSAOTexture;
	}
}

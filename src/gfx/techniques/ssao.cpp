#include "stdafx.h"
#include "ssao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/rootsignature.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/shadercompiler.h"

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

		m_SSAORS = RHI::CreateRootSignature("SSAO RS", RHI::RSInitializer().Init().AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootConstants(0, 4).AddRootCBV(100));
		m_BlurSSAORS = RHI::CreateRootSignature("Blur SSAO RS", RHI::RSInitializer().Init().AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootConstants(0, 5));

		m_SSAOShader = RHI::CreateShader("ssao.hlsl", "ComputeSSAO", RHI::ShaderType::Compute);
		RHI::SC::Compile(m_SSAOShader);
		{
			RHI::PipelineStateInitializer psoInit = {};
			psoInit.SetRootSignature(m_SSAORS);
			psoInit.SetComputeShader(m_SSAOShader);
			psoInit.SetName("SSAO PSO");
			m_SSAOPSO = RHI::CreatePSO(psoInit);
		}

		m_BlurredSSAOTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "Blurred SSAO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});
		m_BlurSSAOShader = RHI::CreateShader("ssao.hlsl", "BlurSSAO", RHI::ShaderType::Compute);
		RHI::SC::Compile(m_BlurSSAOShader);
		{
			RHI::PipelineStateInitializer psoInit = {};
			psoInit.SetRootSignature(m_BlurSSAORS);
			psoInit.SetComputeShader(m_BlurSSAOShader);
			psoInit.SetName("SSAO PSO");
			m_BlurSSAOPSO = RHI::CreatePSO(psoInit);
		}
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_BlurredSSAOTexture);
		RHI::DestroyShader(m_SSAOShader);
		RHI::DestroyShader(m_BlurSSAOShader);

		RHI::DestroyPSO(m_SSAOPSO);
		RHI::DestroyPSO(m_BlurSSAOPSO);

		RHI::DestroyRootSignature(m_SSAORS);
		RHI::DestroyRootSignature(m_BlurSSAORS);
	}

	void SSAO::Render(SceneRenderer* sceneRenderer, RHI::TextureHandle positionsMap, RHI::TextureHandle sceneDepthMap)
	{
		{
			RHI::BeginProfileEvent("SSAO");
			RHI::SetPipelineState(m_SSAOPSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_UnblurredSSAOTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavHandles, _countof(uavHandles));

			RHI::BindConstants(1, 0, sceneRenderer->Tweaks.SSAORadius);
			RHI::BindConstants(1, 1, sceneRenderer->Tweaks.SSAOPower);
			RHI::BindConstants(1, 2, RM_GET(positionsMap)->SRV());
			RHI::BindConstants(1, 3, RM_GET(sceneDepthMap)->SRV());

			RHI::BindTempConstantBuffer(2, sceneRenderer->SceneInfo);

			RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			RHI::EndProfileEvent("SSAO");
		}

		{
			RHI::BeginProfileEvent("SSAO Blur Texture");
			RHI::SetPipelineState(m_BlurSSAOPSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RM_GET(m_BlurredSSAOTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavHandles, _countof(uavHandles));

			RHI::BindConstants(1, 4, RM_GET(m_UnblurredSSAOTexture)->SRV());

			RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			RHI::EndProfileEvent("SSAO Blur Texture");
		}
	}

	RHI::TextureHandle SSAO::GetBlurredTexture() const
	{
		return m_BlurredSSAOTexture;
	}
}

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

		m_SSAORS = new RHI::RootSignature("SSAO RS");
		m_SSAORS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
		m_SSAORS->AddRootConstants(0, 4);
		m_SSAORS->AddRootCBV(100);
		m_SSAORS->Create();

		m_BlurSSAORS = new RHI::RootSignature("Blur SSAO RS");
		m_BlurSSAORS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
		m_BlurSSAORS->AddRootConstants(0, 5);
		m_BlurSSAORS->Create();

		m_SSAOShader = RHI::CreateShader("ssao.hlsl", "ComputeSSAO", RHI::ShaderType::Compute);
		RHI::SC::Compile(m_SSAOShader);
		{
			RHI::PipelineStateInitializer psoInit = {};
			psoInit.SetRootSignature(m_SSAORS);
			psoInit.SetComputeShader(m_SSAOShader);
			psoInit.SetName("SSAO PSO");
			m_SSAOPSO = new RHI::PipelineStateObject(psoInit);
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
			m_BlurSSAOPSO = new RHI::PipelineStateObject(psoInit);
		}
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_BlurredSSAOTexture);
		RHI::DestroyShader(m_SSAOShader);
		RHI::DestroyShader(m_BlurSSAOShader);

		delete m_SSAORS;
		delete m_SSAOPSO;
		delete m_BlurSSAORS;
		delete m_BlurSSAOPSO;
		
	}

	void SSAO::Render(SceneRenderer* sceneRenderer, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> sceneDepthMap)
	{
		{
			RHI::BeginProfileEvent("SSAO");
			RHI::SetPipelineState(m_SSAOPSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RHI::GetTexture(m_UnblurredSSAOTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavHandles, _countof(uavHandles));

			RHI::BindConstants(1, 0, sceneRenderer->Tweaks.SSAORadius);
			RHI::BindConstants(1, 1, sceneRenderer->Tweaks.SSAOPower);
			RHI::BindConstants(1, 2, RHI::GetTexture(positionsMap)->SRV());
			RHI::BindConstants(1, 3, RHI::GetTexture(sceneDepthMap)->SRV());

			RHI::BindTempConstantBuffer(2, sceneRenderer->SceneInfo);

			RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			RHI::EndProfileEvent("SSAO");
		}

		{
			RHI::BeginProfileEvent("SSAO Blur Texture");
			RHI::SetPipelineState(m_BlurSSAOPSO);

			RHI::DescriptorHandle uavHandles[] =
			{
				RHI::GetTexture(m_BlurredSSAOTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavHandles, _countof(uavHandles));

			RHI::BindConstants(1, 4, RHI::GetTexture(m_UnblurredSSAOTexture)->SRV());

			RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			RHI::EndProfileEvent("SSAO Blur Texture");
		}
	}

	RHI::Handle<RHI::Texture> SSAO::GetBlurredTexture() const
	{
		return m_BlurredSSAOTexture;
	}
}

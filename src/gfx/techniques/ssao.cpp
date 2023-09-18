#include "stdafx.h"
#include "ssao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/rootsignature.h"

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
		m_SSAORS->AddDescriptorTable(100, 1, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
		m_SSAORS->Create();

		m_BlurSSAORS = new RHI::RootSignature("Blur SSAO RS");
		m_BlurSSAORS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
		m_BlurSSAORS->AddRootConstants(0, 5);
		m_BlurSSAORS->Create();

		m_SSAOShader = RHI::CreateShader({
			.ProgramName = "ssao",
			.CsEntryPoint = "ComputeSSAO",
			.RootSignature = m_SSAORS,
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
			.RootSignature = m_BlurSSAORS,
			.Type = RHI::ShaderType::Compute
		});
	}

	SSAO::~SSAO()
	{
		RHI::DestroyTexture(m_UnblurredSSAOTexture);
		RHI::DestroyTexture(m_BlurredSSAOTexture);
		RHI::DestroyShader(m_SSAOShader);
		RHI::DestroyShader(m_BlurSSAOShader);

		delete m_SSAORS;
		delete m_BlurSSAORS;
	}

	void SSAO::Render(SceneRenderer* sceneRenderer, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> sceneDepthMap)
	{
		{
			RHI::BeginProfileEvent("SSAO");
			RHI::BindShader(m_SSAOShader);

			RHI::DescriptorHandle uavHandles[] =
			{
				RHI::GetTexture(m_UnblurredSSAOTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavHandles, _countof(uavHandles));

			RHI::BindConstants(1, 0, sceneRenderer->Tweaks.SSAORadius);
			RHI::BindConstants(1, 1, sceneRenderer->Tweaks.SSAOPower);
			RHI::BindConstants(1, 2, RHI::GetTexture(positionsMap)->SRV());
			RHI::BindConstants(1, 3, RHI::GetTexture(sceneDepthMap)->SRV());

			RHI::DescriptorHandle cbvHandles[] =
			{
				RHI::GetBuffer(sceneRenderer->GetSceneInfoBuffer())->CBVHandle
			};
			RHI::BindTempDescriptorTable(2, cbvHandles, _countof(cbvHandles));

			RHI::Dispatch(RHI::GetBackbufferWidth() / 16, RHI::GetBackbufferHeight() / 16, 1);
			RHI::EndProfileEvent("SSAO");
		}

		{
			RHI::BeginProfileEvent("SSAO Blur Texture");
			RHI::BindShader(m_BlurSSAOShader);

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

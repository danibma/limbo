#include "stdafx.h"
#include "rtao.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/rootsignature.h"

namespace limbo::Gfx
{
	RTAO::RTAO()
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

		m_FinalTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Final Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});

		m_CommonRS = new RHI::RootSignature("RTAO Common RS");
		m_CommonRS->AddRootSRV(0);
		m_CommonRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
		m_CommonRS->AddRootCBV(100);
		m_CommonRS->AddRootConstants(0, 3);
		m_CommonRS->Create();

		m_RTAOShader = RHI::CreateShader({
			.ProgramName = "RTAO",
			.RootSignature = m_CommonRS,
			.Libs = {
				{
					.LibName = "raytracing/rtao",
					.HitGroupsDescriptions = 
					{
						{
							.HitGroupExport = L"RTAOHitGroup",
							.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
							.AnyHitShaderImport = L"RTAOAnyHit",
						}
					},
					.Exports = 
					{
						{ .Name = L"RTAORayGen" },
						{ .Name = L"RTAOAnyHit" },
						{ .Name = L"RTAOMiss" }
					},
					
				},
			},
			.ShaderConfig = {
				.MaxPayloadSizeInBytes = sizeof(float), // AOPayload
				.MaxAttributeSizeInBytes = sizeof(float2) // float2 barycentrics
			},
			.Type = RHI::ShaderType::RayTracing,
		});

		m_DenoiseRTAOShader = RHI::CreateShader({
			.ProgramName = "raytracing/rtaoaccumulate",
			.CsEntryPoint = "RTAOAccumulate",
			.RootSignature = m_CommonRS,
			.Type = RHI::ShaderType::Compute
		});
	}

	RTAO::~RTAO()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_RTAOShader.IsValid())
			DestroyShader(m_RTAOShader);
		if (m_DenoiseRTAOShader.IsValid())
			DestroyShader(m_DenoiseRTAOShader);
		if (m_NoisedTexture.IsValid())
			DestroyTexture(m_NoisedTexture);
		if (m_PreviousFrame.IsValid())
			DestroyTexture(m_PreviousFrame);

		delete m_CommonRS;
	}

	void RTAO::Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, RHI::Handle<RHI::Texture> positionsMap, RHI::Handle<RHI::Texture> normalsMap)
	{
		if (sceneRenderer->SceneInfo.PrevView != sceneRenderer->SceneInfo.View)
		{
			RHI::DestroyTexture(m_PreviousFrame);
			PreparePreviousFrameTexture();
		}

		{
			RHI::BeginProfileEvent("RTAO");
			RHI::BindShader(m_RTAOShader);

			RHI::ShaderBindingTable SBT(m_RTAOShader);
			SBT.BindRayGen(L"RTAORayGen");
			SBT.BindMissShader(L"RTAOMiss");
			SBT.BindHitGroup(L"RTAOHitGroup");

			RHI::BindRootSRV(0, sceneAS->GetTLASBuffer()->Resource->GetGPUVirtualAddress());

			RHI::DescriptorHandle uavHandles[] =
			{
				RHI::GetTexture(m_NoisedTexture)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(1, uavHandles, 1);

			RHI::DescriptorHandle cbvHandles[] =
			{
				RHI::GetBuffer(sceneRenderer->GetSceneInfoBuffer())->CBVHandle
			};
			RHI::BindTempDescriptorTable(2, cbvHandles, 1); // todo: make a BindTempCBV

			RHI::BindConstants(3, 0, sceneRenderer->Tweaks.SSAORadius);
			RHI::BindConstants(3, 1, sceneRenderer->Tweaks.SSAOPower);
			RHI::BindConstants(3, 2, sceneRenderer->Tweaks.RTAOSamples);
			RHI::BindConstants(3, 3, RHI::GetTexture(positionsMap)->SRV());
			RHI::BindConstants(3, 4, RHI::GetTexture(normalsMap)->SRV());
			RHI::DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
			RHI::EndProfileEvent("RTAO");
		}

		{
			RHI::BeginProfileEvent("RTAO Denoise");
			RHI::BindShader(m_DenoiseRTAOShader);

			RHI::DescriptorHandle uavHandles[] =
			{
				RHI::GetTexture(m_FinalTexture)->UAVHandle[0],
			};
			RHI::BindTempDescriptorTable(1, uavHandles, _countof(uavHandles));

			RHI::BindConstants(3, 0, m_AccumCount);
			RHI::BindConstants(3, 1, RHI::GetTexture(m_PreviousFrame)->SRV());
			RHI::BindConstants(3, 2, RHI::GetTexture(m_NoisedTexture)->SRV());
			RHI::Dispatch(RHI::GetBackbufferWidth() / 8, RHI::GetBackbufferHeight() / 8, 1);
			RHI::EndProfileEvent("RTAO Denoise");
		}

		++m_AccumCount;

		RHI::GetCommandContext()->InsertUAVBarrier(m_FinalTexture);
		RHI::CopyTextureToTexture(m_FinalTexture, m_PreviousFrame);
	}

	RHI::Handle<RHI::Texture> RTAO::GetFinalTexture() const
	{
		return m_FinalTexture;
	}

	void RTAO::PreparePreviousFrameTexture()
	{
		m_AccumCount = 0;
		m_PreviousFrame = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "RTAO Previous Frame",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});
	}
}

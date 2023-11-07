#include "stdafx.h"
#include "psocache.h"

#include "shaderinterop.h"
#include "shaderscache.h"
#include "core/timer.h"
#include "rhi/device.h"
#include "rhi/resourcemanager.h"

namespace limbo::Gfx::PSOCache
{
	std::unordered_map<PipelineID, RHI::PSOHandle> s_Pipelines;
	std::vector<RHI::RootSignatureHandle> s_RootSignatures;

	void CompilePSOs()
	{
		Core::Timer t;

		// Generate mips
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Generate Mips RS", RHI::RSSpec().Init()
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
										  .AddRootConstants(0, 4));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_GenerateMips))
				.SetName("Generate Mips PSO")
				.SetRootSignature(rs);
			s_Pipelines[PipelineID::GenerateMips] = RHI::CreatePSO(psoInit);
		}

		// Deferred shading
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Deferred Shading RS", RHI::RSSpec().Init()
										  .AddRootConstants(0, 1)
										  .AddRootCBV(100));

			constexpr RHI::Format deferredShadingFormats[] = {
				RHI::Format::RGBA16_SFLOAT,
				RHI::Format::RGBA16_SFLOAT,
				RHI::Format::RGBA16_SFLOAT,
				RHI::Format::RGBA16_SFLOAT,
				RHI::Format::RGBA16_SFLOAT,
				RHI::Format::RGBA16_SFLOAT
			};

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetVertexShader(ShadersCache::Get(ShaderID::VS_GBuffer))
				.SetPixelShader(ShadersCache::Get(ShaderID::PS_GBuffer))
				.SetRootSignature(rs)
				.SetRenderTargetFormats(deferredShadingFormats, RHI::Format::D32_SFLOAT)
				.SetDepthStencilDesc(RHI::TStaticDepthStencilState<true, false, D3D12_COMPARISON_FUNC_GREATER>::GetRHI())
				.SetName("Deferred Shading PSO");
			s_Pipelines[PipelineID::DeferredShading] = RHI::CreatePSO(psoInit);
		}

		// Sky
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Skybox RS", RHI::RSSpec().Init()
										  .AddRootCBV(100)
										  .AddRootConstants(0, 1)
										  .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetVertexShader(ShadersCache::Get(ShaderID::VS_Sky))
				.SetPixelShader(ShadersCache::Get(ShaderID::PS_Sky))
				.SetRootSignature(rs)
				.SetRenderTargetFormats({ RHI::Format::RGBA16_SFLOAT }, RHI::Format::UNKNOWN)
				.SetDepthStencilDesc(RHI::TStaticDepthStencilState<false>::GetRHI())
				.SetInputLayout({ { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT } })
				.SetName("Skybox PSO");
			s_Pipelines[PipelineID::Skybox] = RHI::CreatePSO(psoInit);
		}

		// PBR Lighting
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Lighting RS", RHI::RSSpec().Init()
										  .AddRootConstants(0, 7)
										  .AddRootCBV(100)
										  .AddRootCBV(101)
										  .AddRootConstants(1, 10));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetVertexShader(ShadersCache::Get(ShaderID::VS_Quad))
				.SetPixelShader(ShadersCache::Get(ShaderID::PS_DeferredLighting))
				.SetRootSignature(rs)
				.SetRenderTargetFormats({ RHI::Format::RGBA16_SFLOAT }, RHI::Format::UNKNOWN)
				.SetDepthStencilDesc(RHI::TStaticDepthStencilState<false>::GetRHI())
				.SetName("PBR Lighting PSO");
			s_Pipelines[PipelineID::PBRLighting] = RHI::CreatePSO(psoInit);
		}

		// Composite
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Composite RS", RHI::RSSpec().Init()
										  .AddRootConstants(0, 3)
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetVertexShader(ShadersCache::Get(ShaderID::VS_Quad))
				.SetPixelShader(ShadersCache::Get(ShaderID::PS_Composite))
				.SetRootSignature(rs)
				.SetRenderTargetFormats({ RHI::GetSwapchainFormat() }, RHI::Format::D32_SFLOAT)
				.SetName("Composite PSO");
			s_Pipelines[PipelineID::Composite] = RHI::CreatePSO(psoInit);
		}

		// EquirectToCubemap
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("EquirectToCubemap RS", RHI::RSSpec().Init()
																	   .AddRootConstants(0, 1)
																	   .AddRootConstants(1, 1)
																	   .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_EquirectToCubemap))
				.SetRootSignature(rs);
			s_Pipelines[PipelineID::EquirectToCubemap] = RHI::CreatePSO(psoInit);
		}

		// DrawIrradianceMap
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("DrawIrradianceMap RS", RHI::RSSpec().Init()
										  .AddRootConstants(0, 1)
										  .AddRootConstants(1, 1)
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_DrawIrradianceMap))
				.SetRootSignature(rs);
			s_Pipelines[PipelineID::DrawIrradianceMap] = RHI::CreatePSO(psoInit);
		}

		// PreFilterEnvMap
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("PreFilterEnvMap RS", RHI::RSSpec().Init()
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
										  .AddRootConstants(0, 2)
										  .AddRootConstants(1, 1)
										  .AddRootConstants(2, 1));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_PreFilterEnvMap))
				.SetRootSignature(rs);
			s_Pipelines[PipelineID::PreFilterEnvMap] = RHI::CreatePSO(psoInit);
		}

		// ComputeBRDFLUT
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("ComputeBRDFLUT RS", RHI::RSSpec().Init()
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_ComputeBRDFLUT))
				.SetRootSignature(rs);
			s_Pipelines[PipelineID::ComputeBRDFLUT] = RHI::CreatePSO(psoInit);
		}

		// ShadowMapping
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Shadow Mapping Common RS", RHI::RSSpec().Init()
										  .AddRootCBV(100)
										  .AddRootCBV(101)
										  .AddRootConstants(0, 2));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetVertexShader(ShadersCache::Get(ShaderID::VS_ShadowMapping))
				.SetPixelShader(ShadersCache::Get(ShaderID::PS_ShadowMapping))
				.SetRootSignature(rs)
				.SetRenderTargetFormats({}, RHI::Format::D32_SFLOAT)
				.SetRasterizerDesc(RHI::TStaticRasterizerState<D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_FRONT, true, 7500, 1.0f, 0.0f, false>::GetRHI())
				.SetName("Shadow Map PSO");
			s_Pipelines[PipelineID::ShadowMapping] = RHI::CreatePSO(psoInit);
		}

		// SSAO
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("SSAO RS", RHI::RSSpec().Init()
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
										  .AddRootConstants(0, 4)
										  .AddRootCBV(100));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_SSAO))
				.SetRootSignature(rs)
				.SetName("SSAO PSO");
			s_Pipelines[PipelineID::SSAO] = RHI::CreatePSO(psoInit);
		}

		// SSAOBlur
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("Blur SSAO RS", RHI::RSSpec().Init()
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
										  .AddRootConstants(0, 5));

			RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
				.SetRootSignature(rs)
				.SetComputeShader(ShadersCache::Get(ShaderID::CS_SSAOBlur))
				.SetName("SSAO PSO");
			s_Pipelines[PipelineID::SSAOBlur] = RHI::CreatePSO(psoInit);
		}

		// PathTracing
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("PT Common RS", RHI::RSSpec().Init()
										  .AddRootSRV(0)
										  .AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
										  .AddRootCBV(100));

			RHI::RTPipelineStateSpec psoInit = RHI::RTPipelineStateSpec().Init()
				.SetGlobalRootSignature(rs)
				.SetName("Path Tracer PSO")
				.SetShaderConfig(sizeof(MaterialRayTracingPayload), sizeof(float2) /* BuiltInTriangleIntersectionAttributes */);

			RHI::RTLibSpec pathTracerDesc = RHI::RTLibSpec().Init().AddExport(L"RayGen");
			psoInit.AddLib(ShadersCache::Get(ShaderID::LIB_PathTracer), pathTracerDesc);

			RHI::RTLibSpec materialDesc = RHI::RTLibSpec().Init()
				.AddExport(L"MaterialAnyHit")
				.AddExport(L"MaterialClosestHit")
				.AddExport(L"MaterialMiss")
				.AddHitGroup(L"MaterialHitGroup", L"MaterialAnyHit", L"MaterialClosestHit");
			psoInit.AddLib(ShadersCache::Get(ShaderID::LIB_Material), materialDesc);

			s_Pipelines[PipelineID::PathTracing] = RHI::CreatePSO(psoInit);
		}

		// Full RTAO
		{
			RHI::RootSignatureHandle& rs = s_RootSignatures.emplace_back();
			rs = RHI::CreateRootSignature("RTAO Common RS", RHI::RSSpec().Init().AddRootSRV(0).AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootCBV(100).AddRootConstants(0, 5));

			// RTAO
			{
				RHI::RTLibSpec libDesc = RHI::RTLibSpec().Init()
					.AddExport(L"RTAORayGen")
					.AddExport(L"RTAOAnyHit")
					.AddExport(L"RTAOMiss")
					.AddHitGroup(L"RTAOHitGroup", L"RTAOAnyHit");

				RHI::RTPipelineStateSpec psoInit = RHI::RTPipelineStateSpec().Init()
					.SetGlobalRootSignature(rs)
					.AddLib(ShadersCache::Get(ShaderID::LIB_RTAO), libDesc)
					.SetShaderConfig(sizeof(float) /* AOPayload */, sizeof(float2) /* BuiltInTriangleIntersectionAttributes */)
					.SetName("RTAO PSO");
				s_Pipelines[PipelineID::RTAO] = RHI::CreatePSO(psoInit);
			}

			// RTAO Accumulate
			{
				RHI::PipelineStateSpec psoInit = RHI::PipelineStateSpec().Init()
					.SetRootSignature(rs)
					.SetComputeShader(ShadersCache::Get(ShaderID::CS_RTAOAccumulate))
					.SetName("RTAO Accumulate PSO");
				s_Pipelines[PipelineID::RTAOAccumulate] = RHI::CreatePSO(psoInit);
			}
		}

		LB_LOG("Took %.2fs to create all PSOs", t.ElapsedSeconds());
	}

	void DestroyPSOs()
	{
		for (uint8 i = 0; i < ENUM_COUNT<PipelineID>(); ++i)
		{
			if (!ensure(s_Pipelines.contains((PipelineID)i))) continue;
			RHI::DestroyPSO(s_Pipelines[(PipelineID)i]);
		}

		for (RHI::RootSignatureHandle rs : s_RootSignatures)
			RHI::DestroyRootSignature(rs);
	}

	RHI::PSOHandle Get(PipelineID pipelineID)
	{
		ENSURE_RETURN(!s_Pipelines.contains(pipelineID), RHI::PSOHandle());
		return s_Pipelines[pipelineID];
	}
}

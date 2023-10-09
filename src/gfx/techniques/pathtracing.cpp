#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/rootsignature.h"
#include "gfx/rhi/shaderbindingtable.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/shadercompiler.h"

namespace limbo::Gfx
{
	PathTracing::PathTracing()
	{
		m_FinalTexture = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "Path Tracing Final Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA8_UNORM,
			.Type = RHI::TextureType::Texture2D,
		});

		m_CommonRS = RHI::CreateRootSignature("PT Common RS", RHI::RSInitializer().Init().AddRootSRV(0).AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootCBV(100));

		m_PathTracerLib = RHI::CreateShader("raytracing/pathtracer.hlsl", "", RHI::ShaderType::Lib);
		RHI::SC::Compile(m_PathTracerLib);
		m_MaterialLib = RHI::CreateShader("raytracing/materiallib.hlsl", "", RHI::ShaderType::Lib);
		RHI::SC::Compile(m_MaterialLib);

		{

			RHI::RaytracingPipelineStateInitializer psoInit = {};
			psoInit.SetGlobalRootSignature(m_CommonRS);
			psoInit.SetName("Path Tracer PSO");
			psoInit.SetShaderConfig(sizeof(MaterialRayTracingPayload), sizeof(float2) /* BuiltInTriangleIntersectionAttributes */);

			RHI::RaytracingLibDesc pathTracerDesc = {};
			pathTracerDesc.AddExport(L"RayGen");
			psoInit.AddLib(m_PathTracerLib, pathTracerDesc);

			RHI::RaytracingLibDesc materialDesc = {};
			materialDesc.AddExport(L"MaterialAnyHit");
			materialDesc.AddExport(L"MaterialClosestHit");
			materialDesc.AddExport(L"MaterialMiss");
			materialDesc.AddHitGroup(L"MaterialHitGroup", L"MaterialAnyHit", L"MaterialClosestHit");
			psoInit.AddLib(m_MaterialLib, materialDesc);

			m_PSO = RHI::CreatePSO(psoInit);
		}
	}

	PathTracing::~PathTracing()
	{
		if (m_FinalTexture.IsValid())
			RHI::DestroyTexture(m_FinalTexture);
		if (m_PathTracerLib.IsValid())
			RHI::DestroyShader(m_PathTracerLib);
		if (m_MaterialLib.IsValid())
			RHI::DestroyShader(m_MaterialLib);

		RHI::DestroyPSO(m_PSO);
		RHI::DestroyRootSignature(m_CommonRS);
	}

	void PathTracing::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera)
	{
		cmd->BeginProfileEvent("Path Tracing");
		cmd->SetPipelineState(m_PSO);

		RHI::ShaderBindingTable SBT(m_PSO);
		SBT.BindRayGen(L"RayGen");
		SBT.BindMissShader(L"MaterialMiss");
		SBT.BindHitGroup(L"MaterialHitGroup");

		cmd->BindRootSRV(0, sceneAS->GetTLASBuffer()->Resource->GetGPUVirtualAddress());

		RHI::DescriptorHandle uavHandles[] = { RM_GET(m_FinalTexture)->UAVHandle[0] };
		cmd->BindTempDescriptorTable(1, uavHandles, 1);

		cmd->BindTempConstantBuffer(2, sceneRenderer->SceneInfo);
		cmd->DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		cmd->EndProfileEvent("Path Tracing");
	}

	RHI::TextureHandle PathTracing::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}

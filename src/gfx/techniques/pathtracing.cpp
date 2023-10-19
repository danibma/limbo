#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/psocache.h"
#include "gfx/scenerenderer.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/shaderbindingtable.h"
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
	}

	PathTracing::~PathTracing()
	{
		if (m_FinalTexture.IsValid())
			RHI::DestroyTexture(m_FinalTexture);
	}

	void PathTracing::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera)
	{
		RHI::PSOHandle pso = PSOCache::Get(PipelineID::PathTracing);

		cmd->BeginProfileEvent("Path Tracing");
		cmd->SetPipelineState(pso);

		RHI::ShaderBindingTable SBT(pso);
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

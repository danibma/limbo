#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/shaderbindingtable.h"
#include "gfx/rhi/shadercompiler.h"

namespace limbo::Gfx
{
	PathTracing::PathTracing()
		: RenderTechnique("Path Tracing")
	{
	}

	PathTracing::~PathTracing()
	{
		RHI::DestroyTexture(m_AccumulationBuffer);
	}

	bool PathTracing::Init()
	{
		m_AccumulationBuffer = RHI::CreateTexture({
			.Width = RHI::GetBackbufferWidth(),
			.Height = RHI::GetBackbufferHeight(),
			.DebugName = "PathTracer Accumulation Buffer",
			.Flags = RHI::TextureUsage::UnorderedAccess,
			.Format = RHI::Format::RGBA32_SFLOAT,
		});
		
		return true;
	}

	void PathTracing::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		if (context.SceneInfo.PrevView != context.SceneInfo.View)
			m_AccumulatedFrames = 1;
		
		RHI::PSOHandle pso = PSOCache::Get(PipelineID::PathTracing);

		cmd.BeginProfileEvent("Path Tracing");
		cmd.SetPipelineState(pso);

		RHI::ShaderBindingTable SBT(pso);
		SBT.BindRayGen(L"RayGen");
		SBT.BindMissShader(L"MaterialMiss");
		SBT.BindHitGroup(L"MaterialHitGroup");

		cmd.BindRootSRV(0, context.SceneAccelerationStructure.GetTLASBuffer()->Resource->GetGPUVirtualAddress());

		RHI::DescriptorHandle uavHandles[] =
		{
			RM_GET(context.SceneTextures.PreCompositeSceneTexture)->UAVHandle[0],
			RM_GET(m_AccumulationBuffer)->UAVHandle[0]
		};
		cmd.BindTempDescriptorTable(1, uavHandles, 2);

		cmd.BindTempConstantBuffer(2, context.SceneInfo);
		cmd.BindConstants(3, 0, m_AccumulatedFrames++);
		cmd.DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		cmd.EndProfileEvent("Path Tracing");
	}
}

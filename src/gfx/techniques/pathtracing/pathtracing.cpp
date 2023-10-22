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

	bool PathTracing::Init()
	{
		return true;
	}

	void PathTracing::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		RHI::PSOHandle pso = PSOCache::Get(PipelineID::PathTracing);

		cmd.BeginProfileEvent("Path Tracing");
		cmd.SetPipelineState(pso);

		RHI::ShaderBindingTable SBT(pso);
		SBT.BindRayGen(L"RayGen");
		SBT.BindMissShader(L"MaterialMiss");
		SBT.BindHitGroup(L"MaterialHitGroup");

		cmd.BindRootSRV(0, context.SceneAccelerationStructure.GetTLASBuffer()->Resource->GetGPUVirtualAddress());

		RHI::DescriptorHandle uavHandles[] = { RM_GET(context.SceneTextures.PreCompositeSceneTexture)->UAVHandle[0] };
		cmd.BindTempDescriptorTable(1, uavHandles, 1);

		cmd.BindTempConstantBuffer(2, context.SceneInfo);
		cmd.DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		cmd.EndProfileEvent("Path Tracing");
	}
}

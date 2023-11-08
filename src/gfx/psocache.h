#pragma once
#include "rhi/pipelinestateobject.h"

namespace limbo::Gfx
{
	enum class PipelineID : uint8
	{
		GenerateMips,
		DeferredShading,
		Skybox,
		PBRLighting,
		Composite,
		ShadowMapping,
		SSAO,
		Blur_V,
		Blur_H,
		EquirectToCubemap,
		DrawIrradianceMap,
		PreFilterEnvMap,
		ComputeBRDFLUT,

		PathTracing,
		RTAO,
		RTAOAccumulate,

		MAX
	};

	namespace PSOCache
	{
		void CompilePSOs();
		void DestroyPSOs();

		RHI::PSOHandle Get(PipelineID pipelineID);
	}
}

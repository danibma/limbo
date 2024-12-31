#pragma once
#include "rhi/shader.h"

namespace limbo::Gfx
{
	enum class ShaderID : uint8
	{
		CS_GenerateMips,
		CS_SSAO,
		CS_Blur_V,
		CS_Blur_H,
		CS_EquirectToCubemap,
		CS_DrawIrradianceMap,
		CS_PreFilterEnvMap,
		CS_ComputeBRDFLUT,
		CS_RTAOAccumulate,

		VS_GBuffer,
		VS_Sky,
		VS_Quad,
		VS_ShadowMapping,

		MS_GBuffer,

		PS_GBuffer,
		PS_Sky,
		PS_DeferredLighting,
		PS_Composite,

		LIB_PathTracer,
		LIB_RTAO,

		MAX
	};

	namespace ShadersCache
	{
		void CompileShaders();
		void DestroyShaders();

		RHI::ShaderHandle Get(ShaderID shaderID);
	}
}

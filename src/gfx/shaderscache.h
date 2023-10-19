#pragma once
#include "rhi/shader.h"

namespace limbo::Gfx
{
	enum class ShaderID : uint8
	{
		CS_GenerateMips,
		CS_SSAO,
		CS_SSAOBlur,
		CS_EquirectToCubemap,
		CS_DrawIrradianceMap,
		CS_PreFilterEnvMap,
		CS_ComputeBRDFLUT,
		CS_RTAOAccumulate,

		VS_GBuffer,
		VS_Sky,
		VS_Quad,
		VS_ShadowMapping,

		PS_GBuffer,
		PS_Sky,
		PS_Lighting,
		PS_Composite,
		PS_ShadowMapping,

		LIB_PathTracer,
		LIB_Material,
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

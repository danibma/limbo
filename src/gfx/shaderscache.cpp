#include "stdafx.h"
#include "shaderscache.h"

#include "core/timer.h"
#include "rhi/resourcemanager.h"
#include "rhi/shadercompiler.h"

namespace limbo::Gfx::ShadersCache
{
	std::unordered_map<ShaderID, RHI::Handle<RHI::Shader>> s_Shaders;

	const char* GetShaderFile(ShaderID shaderID)
	{
		switch (shaderID)
		{
		case ShaderID::CS_GenerateMips:
			return "generatemips.hlsl";
		case ShaderID::CS_SSAO:
			return "ssao.hlsl";
		case ShaderID::CS_Blur_V:
		case ShaderID::CS_Blur_H:
			return "blur.hlsl";
		case ShaderID::CS_EquirectToCubemap:
		case ShaderID::CS_DrawIrradianceMap:
		case ShaderID::CS_PreFilterEnvMap:
		case ShaderID::CS_ComputeBRDFLUT:
			return "ibl.hlsl";
		case ShaderID::VS_GBuffer:
		case ShaderID::PS_GBuffer:
		case ShaderID::MS_GBuffer:
			return "gbuffer.hlsl";
		case ShaderID::VS_Sky:
		case ShaderID::PS_Sky:
			return "sky.hlsl";
		case ShaderID::VS_Quad:
			return "quad.hlsl";
		case ShaderID::VS_ShadowMapping:
			return "shadowmap.hlsl";
		case ShaderID::PS_DeferredLighting:
			return "deferredlighting.hlsl";
		case ShaderID::PS_Composite:
			return "scenecomposite.hlsl";
		case ShaderID::LIB_PathTracer:
			return "raytracing/pathtracer.hlsl";
		case ShaderID::LIB_RTAO:
			return "raytracing/rtao.hlsl";
		case ShaderID::CS_RTAOAccumulate:
			return "raytracing/rtaoaccumulate.hlsl";
		case ShaderID::MAX:
		default:
			ensure(false);
			return "";
		}
	}

	const char* GetEntryPoint(ShaderID shaderID)
	{
		switch (shaderID)
		{
		case ShaderID::CS_GenerateMips:
			return "GenerateMipCS";
		case ShaderID::CS_SSAO:
			return "ComputeSSAOCS";
		case ShaderID::CS_Blur_V:
			return "Blur_VerticalCS";
		case ShaderID::CS_Blur_H:
			return "Blur_HorizontalCS";
		case ShaderID::CS_EquirectToCubemap:
			return "EquirectToCubemapCS";
		case ShaderID::CS_DrawIrradianceMap:
			return "DrawIrradianceMapCS";
		case ShaderID::CS_PreFilterEnvMap:
			return "PreFilterEnvMapCS";
		case ShaderID::CS_ComputeBRDFLUT:
			return "ComputeBRDFLUTCS";
		case ShaderID::CS_RTAOAccumulate:
			return "RTAOAccumulateCS";
		case ShaderID::VS_GBuffer:
		case ShaderID::VS_Sky:
		case ShaderID::VS_Quad:
		case ShaderID::VS_ShadowMapping:
			return "MainVS";
		case ShaderID::MS_GBuffer:
			return "MainMS";
		case ShaderID::PS_GBuffer:
		case ShaderID::PS_Sky:
		case ShaderID::PS_DeferredLighting:
		case ShaderID::PS_Composite:
			return "MainPS";
		case ShaderID::LIB_PathTracer:
		case ShaderID::LIB_RTAO:
			return "";
		case ShaderID::MAX:
		default:
			ensure(false);
			return "";
		}
	}

	RHI::ShaderType GetShaderType(ShaderID shaderID)
	{
		switch (shaderID)
		{
		case ShaderID::CS_GenerateMips:
		case ShaderID::CS_SSAO:
		case ShaderID::CS_Blur_V:
		case ShaderID::CS_Blur_H:
		case ShaderID::CS_EquirectToCubemap:
		case ShaderID::CS_DrawIrradianceMap:
		case ShaderID::CS_PreFilterEnvMap:
		case ShaderID::CS_ComputeBRDFLUT:
		case ShaderID::CS_RTAOAccumulate:
			return RHI::ShaderType::Compute;
		case ShaderID::VS_GBuffer:
		case ShaderID::VS_Sky:
		case ShaderID::VS_Quad:
		case ShaderID::VS_ShadowMapping:
			return RHI::ShaderType::Vertex;
		case ShaderID::MS_GBuffer:
			return RHI::ShaderType::Mesh;
		case ShaderID::PS_GBuffer:
		case ShaderID::PS_Sky:
		case ShaderID::PS_DeferredLighting:
		case ShaderID::PS_Composite:
			return RHI::ShaderType::Pixel;
		case ShaderID::LIB_PathTracer:
		case ShaderID::LIB_RTAO:
			return RHI::ShaderType::Lib;
		case ShaderID::MAX:
		default:
			ensure(false);
			return RHI::ShaderType::MAX;
		}
	}

	void CompileShaders()
	{
		Core::Timer t;
		for (uint8 i = 0; i < ENUM_COUNT<ShaderID>(); ++i)
		{
			ShaderID shaderID = (ShaderID)i;
			s_Shaders[shaderID] = RHI::CreateShader(GetShaderFile(shaderID), GetEntryPoint(shaderID), GetShaderType(shaderID));
			RHI::SC::Compile(s_Shaders[shaderID]);
		}
		LB_LOG("Compiled all shaders in %.2fs", t.ElapsedSeconds());
	}

	void DestroyShaders()
	{
		for (uint8 i = 0; i < ENUM_COUNT<ShaderID>(); ++i)
		{
			if (!ensure(s_Shaders.contains((ShaderID)i))) continue;
			RHI::DestroyShader(s_Shaders[(ShaderID)i], true);
		}
	}

	RHI::ShaderHandle Get(ShaderID shaderID)
	{
		ENSURE_RETURN(!s_Shaders.contains(shaderID), RHI::ShaderHandle());
		return s_Shaders[shaderID];
	}
}

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
			return "gbuffer.hlsl";
		case ShaderID::VS_Sky:
		case ShaderID::PS_Sky:
			return "sky.hlsl";
		case ShaderID::VS_Quad:
			return "quad.hlsl";
		case ShaderID::VS_ShadowMapping:
		case ShaderID::PS_ShadowMapping:
			return "shadowmap.hlsl";
		case ShaderID::PS_DeferredLighting:
			return "deferredlighting.hlsl";
		case ShaderID::PS_Composite:
			return "scenecomposite.hlsl";
		case ShaderID::LIB_PathTracer:
			return "raytracing/pathtracer.hlsl";
		case ShaderID::LIB_Material:
			return "raytracing/materiallib.hlsl";
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
			return "GenerateMip";
		case ShaderID::CS_SSAO:
			return "ComputeSSAO";
		case ShaderID::CS_Blur_V:
			return "Blur_Vertical";
		case ShaderID::CS_Blur_H:
			return "Blur_Horizontal";
		case ShaderID::CS_EquirectToCubemap:
			return "EquirectToCubemap";
		case ShaderID::CS_DrawIrradianceMap:
			return "DrawIrradianceMap";
		case ShaderID::CS_PreFilterEnvMap:
			return "PreFilterEnvMap";
		case ShaderID::CS_ComputeBRDFLUT:
			return "ComputeBRDFLUT";
		case ShaderID::CS_RTAOAccumulate:
			return "RTAOAccumulate";
		case ShaderID::VS_GBuffer:
		case ShaderID::VS_Sky:
		case ShaderID::VS_Quad:
		case ShaderID::VS_ShadowMapping:
			return "VSMain";
		case ShaderID::PS_GBuffer:
		case ShaderID::PS_Sky:
		case ShaderID::PS_DeferredLighting:
		case ShaderID::PS_Composite:
		case ShaderID::PS_ShadowMapping:
			return "PSMain";
		case ShaderID::LIB_PathTracer:
		case ShaderID::LIB_Material:
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
		case ShaderID::PS_GBuffer:
		case ShaderID::PS_Sky:
		case ShaderID::PS_DeferredLighting:
		case ShaderID::PS_Composite:
		case ShaderID::PS_ShadowMapping:
			return RHI::ShaderType::Pixel;
		case ShaderID::LIB_PathTracer:
		case ShaderID::LIB_Material:
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
			RHI::DestroyShader(s_Shaders[(ShaderID)i]);
		}
	}

	RHI::ShaderHandle Get(ShaderID shaderID)
	{
		ENSURE_RETURN(!s_Shaders.contains(shaderID), RHI::ShaderHandle());
		return s_Shaders[shaderID];
	}
}

#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/shaderbindingtable.h"

namespace limbo::Gfx
{
	constexpr const wchar_t* GRayGenShaderName		= L"RayGen";
	constexpr const wchar_t* GPMissShaderName		= L"PrimaryMiss";
	constexpr const wchar_t* GPClosestHitShaderName	= L"PrimaryClosestHit";
	constexpr const wchar_t* GPHitGroupName			= L"PrimaryHitGroup";

	PathTracing::PathTracing()
	{
		m_FinalTexture = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "Path Tracing Final Texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Format::RGBA8_UNORM,
			.Type = TextureType::Texture2D,
		});

		m_RTShader = CreateShader({
			.ProgramName = "pathtracer",
			.HitGroupsDescriptions = {
				{
					.HitGroupExport = GPHitGroupName,
					.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
					.ClosestHitShaderImport = GPClosestHitShaderName,
				}
			},
			.Exports = {
				{ .Name = GRayGenShaderName,	  .ExportToRename = GRayGenShaderName },
				{ .Name = GPMissShaderName,		  .ExportToRename = GPMissShaderName },
				{ .Name = GPClosestHitShaderName, .ExportToRename = GPClosestHitShaderName }
			},
			.Type = ShaderType::RayTracing,
		});
	}

	PathTracing::~PathTracing()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_RTShader.IsValid())
			DestroyShader(m_RTShader);
	}

	void PathTracing::Render(SceneRenderer* sceneRenderer, const FPSCamera& camera)
	{
		BeginEvent("Path Tracing");
		BindShader(m_RTShader);

		ShaderBindingTable SBT(m_RTShader);
		SBT.BindRayGen(GRayGenShaderName);
		SBT.BindMissShader(GPMissShaderName);
		SBT.BindHitGroup(GPHitGroupName);

		sceneRenderer->BindSceneInfo(m_RTShader);
		SetParameter(m_RTShader, "Scene", &m_AccelerationStructure);
		SetParameter(m_RTShader, "RenderTarget", m_FinalTexture);
		//SetParameter(m_RTShader, "LinearWrap", GetDefaultLinearWrapSampler());
		DispatchRays(SBT, GetBackbufferWidth(), GetBackbufferHeight());
		EndEvent();
	}

	void PathTracing::RebuildAccelerationStructure(const std::vector<Scene*>& scenes)
	{
		m_AccelerationStructure.Build(scenes);
	}

	Handle<Texture> PathTracing::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}

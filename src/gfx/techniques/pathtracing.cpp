#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/shaderbindingtable.h"

namespace limbo::Gfx
{
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
			.ProgramName = "rttriangle",
			.HitGroupsDescriptions = {
				{
					.HitGroupExport = L"HitGroupName",
					.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
					.ClosestHitShaderImport = L"MyClosestHitShader",
				}
			},
			.Exports = {
				{.Name = L"MyRaygenShader", .ExportToRename = L"MyRaygenShader"},
				{.Name = L"MyMissShader", .ExportToRename = L"MyMissShader" },
				{.Name = L"MyClosestHitShader", .ExportToRename = L"MyClosestHitShader" }
			},
			.Type = ShaderType::RayTracing,
		});
	}

	PathTracing::~PathTracing()
	{
		DestroyTexture(m_FinalTexture);
		DestroyShader(m_RTShader);
	}

	void PathTracing::Render(const FPSCamera& camera)
	{
		BeginEvent("Path Tracing");
		BindShader(m_RTShader);

		ShaderBindingTable SBT(m_RTShader);
		SBT.BindRayGen("MyRaygenShader");
		SBT.BindMissShader("MyMissShader");
		SBT.BindHitGroup("HitGroupName");

		SetParameter(m_RTShader, "Scene", &m_AccelerationStructure);
		SetParameter(m_RTShader, "RenderTarget", m_FinalTexture);
		SetParameter(m_RTShader, "camPos", camera.Eye);
		SetParameter(m_RTShader, "viewProj", glm::inverse(camera.ViewProj));
		//SetParameter(m_RTShader, "dispatchWidth", GetBackbufferWidth());
		//SetParameter(m_RTShader, "dispatchHeight", GetBackbufferHeight());
		//SetParameter(m_RTShader, "holeSize", 1.0f);
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

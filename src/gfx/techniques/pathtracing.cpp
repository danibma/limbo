#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/scenerenderer.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/shaderbindingtable.h"

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

		m_RTShader = RHI::CreateShader({
			.ProgramName = "Path Tracer",
			.Libs = {
				{
					.LibName = "raytracing/pathtracer",
					.Exports = {
						{ .Name = L"RayGen" },
					}
				},
				// Material Lib
				{
					.LibName = "raytracing/materiallib",
					.HitGroupsDescriptions = {
						{
							.HitGroupExport = L"MaterialHitGroup",
							.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
							.AnyHitShaderImport = L"MaterialAnyHit",
							.ClosestHitShaderImport = L"MaterialClosestHit",
						}
					},
					.Exports = {
						{ .Name = L"MaterialAnyHit" },
						{ .Name = L"MaterialClosestHit" },
						{ .Name = L"MaterialMiss" }
					},
				},
			},
			.ShaderConfig = {
				.MaxPayloadSizeInBytes = sizeof(float) + sizeof(uint) + sizeof(uint) + sizeof(float2) + sizeof(uint), // MaterialPayload
				.MaxAttributeSizeInBytes = sizeof(float2) // float2 barycentrics
			},
			.Type = RHI::ShaderType::RayTracing,
		});
	}

	PathTracing::~PathTracing()
	{
		if (m_FinalTexture.IsValid())
			DestroyTexture(m_FinalTexture);
		if (m_RTShader.IsValid())
			DestroyShader(m_RTShader);
	}

	void PathTracing::Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera)
	{
		RHI::BeginProfileEvent("Path Tracing");
		RHI::BindShader(m_RTShader);

		RHI::ShaderBindingTable SBT(m_RTShader);
		SBT.BindRayGen(L"RayGen");
		SBT.BindMissShader(L"MaterialMiss");
		SBT.BindHitGroup(L"MaterialHitGroup");

		sceneRenderer->BindSceneInfo(m_RTShader);
		RHI::SetParameter(m_RTShader, "Scene", sceneAS);
		RHI::SetParameter(m_RTShader, "RenderTarget", m_FinalTexture);
		RHI::DispatchRays(SBT, RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());
		RHI::EndProfileEvent("Path Tracing");
	}

	RHI::Handle<RHI::Texture> PathTracing::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}

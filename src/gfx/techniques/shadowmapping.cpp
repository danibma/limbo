#include "stdafx.h"
#include "shadowmapping.h"

#include "gfx/profiler.h"
#include "gfx/scene.h"
#include "gfx/scenerenderer.h"
#include "gfx/shaderinterop.h"
#include "gfx/uirenderer.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/rootsignature.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/shadercompiler.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <format>

namespace limbo::Gfx
{
	constexpr uint32 SHADOWMAP_SIZES[SHADOWMAP_CASCADES] =
	{
		8196,
		4096,
		2048,
		1024,
	};

	ShadowMapping::ShadowMapping()
	{
		m_CommonRS = RHI::CreateRootSignature("Shadow Mapping Common RS", RHI::RSInitializer().Init().AddRootCBV(100).AddRootCBV(101).AddRootConstants(0, 2));

		for (int i = 0; i < SHADOWMAP_CASCADES; ++i)
			m_DepthShadowMaps[i] = RHI::CreateTexture(RHI::Tex2DDepth(SHADOWMAP_SIZES[i], SHADOWMAP_SIZES[i], 1.0f));

		m_ShadowMapVS = RHI::CreateShader("shadowmap.hlsl", "VSMain", RHI::ShaderType::Vertex);
		RHI::SC::Compile(m_ShadowMapVS);
		m_ShadowMapPS = RHI::CreateShader("shadowmap.hlsl", "PSMain", RHI::ShaderType::Pixel);
		RHI::SC::Compile(m_ShadowMapPS);

		{
			RHI::PipelineStateInitializer psoInit = {};
			psoInit.SetVertexShader(m_ShadowMapVS);
			psoInit.SetPixelShader(m_ShadowMapPS);
			psoInit.SetRootSignature(m_CommonRS);
			psoInit.SetRenderTargetFormats({}, RHI::Format::D32_SFLOAT);
			psoInit.SetName("Shadow Map PSO");
			m_PSO = RHI::CreatePSO(psoInit);
		}
	}

	ShadowMapping::~ShadowMapping()
	{
		DestroyShader(m_ShadowMapVS);
		DestroyShader(m_ShadowMapPS);

		for (int i = 0; i < SHADOWMAP_CASCADES; ++i)
			DestroyTexture(m_DepthShadowMaps[i]);

		RHI::DestroyPSO(m_PSO);
		RHI::DestroyRootSignature(m_CommonRS);
	}

	void ShadowMapping::Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer)
	{
		if (UI::Globals::bDebugShadowMaps)
			DrawDebugWindow();

		CreateLightMatrices(&sceneRenderer->Camera, sceneRenderer->SceneInfo.SunDirection);

		// Shadow map
		cmd->BeginProfileEvent("Shadow Maps Pass");
		cmd->SetPipelineState(m_PSO);
		cmd->SetPrimitiveTopology();
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
		{
			std::string profileName = std::format("Shadow Cascade {}", cascade);

			cmd->BeginProfileEvent(profileName.c_str());
			cmd->SetRenderTargets({}, m_DepthShadowMaps[cascade]);
			cmd->SetViewport(SHADOWMAP_SIZES[cascade], SHADOWMAP_SIZES[cascade]);

			cmd->ClearDepthTarget(m_DepthShadowMaps[cascade], 1.0f);

			cmd->BindConstants(2, 0, cascade);

			cmd->BindTempConstantBuffer(0, sceneRenderer->SceneInfo);
			cmd->BindTempConstantBuffer(1, m_ShadowData);

			for (const Scene* scene : sceneRenderer->GetScenes())
			{
				scene->IterateMeshes(TOnDrawMesh::CreateLambda([&](const Mesh& mesh)
				{
					cmd->BindConstants(2, 1, mesh.InstanceID);

					cmd->SetIndexBufferView(mesh.IndicesLocation);
					cmd->DrawIndexed((uint32)mesh.IndexCount);
				}));
			}
			cmd->EndProfileEvent(profileName.c_str());
		}
		cmd->EndProfileEvent("Shadow Maps Pass");
}

	void ShadowMapping::DrawDebugWindow()
	{
		ImGui::Begin("Shadow Map Debug", &UI::Globals::bDebugShadowMaps);
		ImGui::Checkbox("Show Shadow Cascades", &UI::Globals::bShowShadowCascades);
		ImGui::SliderInt("Shadow Cascade", &UI::Globals::ShadowCascadeIndex, 0, SHADOWMAP_CASCADES - 1);
		ImGui::Image((ImTextureID)RM_GET(m_DepthShadowMaps[UI::Globals::ShadowCascadeIndex])->TextureID(), ImVec2(512, 512));
		ImGui::End();
	}

	const ShadowData& ShadowMapping::GetShadowData() const
	{
		return m_ShadowData;
	}

	/*
		Calculate frustum split depths and matrices for the shadow map cascades
		Based on https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
	*/
	void ShadowMapping::CreateLightMatrices(const FPSCamera* camera, float3 lightDirection)
	{
		float cascadeSplits[SHADOWMAP_CASCADES];

		float nearClip = camera->NearZ;
		float farClip = camera->FarZ;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32 i = 0; i < SHADOWMAP_CASCADES; i++) 
		{
			float p		  = (i + 1) / static_cast<float>(SHADOWMAP_CASCADES);
			float log	  = minZ * glm::pow(ratio, p);
			float uniform = minZ + range * p;
			float d		  = m_CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
		{
			float3 frustumCorners[8] =
			{
				float3(-1.0f,  1.0f, 0.0f),
				float3( 1.0f,  1.0f, 0.0f),
				float3( 1.0f, -1.0f, 0.0f),
				float3(-1.0f, -1.0f, 0.0f),
				float3(-1.0f,  1.0f, 1.0f),
				float3( 1.0f,  1.0f, 1.0f),
				float3( 1.0f, -1.0f, 1.0f),
				float3(-1.0f, -1.0f, 1.0f),
			};

			float splitDist = cascadeSplits[cascade];

			// Project frustum corners into world space
			float4x4 invCam = glm::inverse(camera->ViewProj);
			for (uint32 i = 0; i < 8; i++) 
			{
				float4 invCorner = invCam * float4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32 i = 0; i < 4; i++) 
			{
				float3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			float3 frustumCenter = float3(0.0f);
			for (uint32_t i = 0; i < 8; i++) 
				frustumCenter += frustumCorners[i];
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) 
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = glm::normalize(-lightDirection);
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::orthoZO(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

			// Store split distance and matrix in cascade
			m_ShadowData.SplitDepth[cascade]    = (camera->NearZ + splitDist * clipRange) * -1.0f;
			m_ShadowData.LightViewProj[cascade] = lightOrthoMatrix * lightViewMatrix;
			m_ShadowData.ShadowMap[cascade]		= RM_GET(m_DepthShadowMaps[cascade])->SRV();

			lastSplitDist = cascadeSplits[cascade];
		}
	}
}

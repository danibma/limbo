#include "stdafx.h"
#include "shadowmapping.h"

#include "gfx/profiler.h"
#include "gfx/scene.h"
#include "gfx/scenerenderer.h"
#include "gfx/shaderinterop.h"
#include "gfx/uirenderer.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	constexpr uint32 SHADOWMAP_SIZES[SHADOWMAP_CASCADES] =
	{
		4096,
		2048,
		2048,
		2048,
	};

	ShadowMapping::ShadowMapping()
	{
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
		{
			std::string debugName = std::format("Shadow Cascade [{}]", cascade);

			m_ShadowMapShaders[cascade] = RHI::CreateShader({
				.ProgramName = "shadowmap",
				.RTSize = { SHADOWMAP_SIZES[cascade], SHADOWMAP_SIZES[cascade] },
				.RTFormats = {
					{
						.RTFormat = RHI::Format::RGBA32_SFLOAT,
						.DebugName = debugName.c_str(),
					}
				},
				.DepthFormat = {
					.RTFormat = RHI::Format::D32_SFLOAT,
					.DebugName = debugName.c_str(),
				},
				.CullMode = D3D12_CULL_MODE_NONE,
				.DepthClip = false,
				.Type = RHI::ShaderType::Graphics
			});
		}

		for (uint8 i = 0; i < RHI::NUM_BACK_BUFFERS; ++i)
		{
			std::string debugName = std::format("CascadedDataBuffer[{}]", i);
			m_ShadowDataBuffer[i] = RHI::CreateBuffer({
				.DebugName = debugName.c_str(),
				.ByteSize = sizeof(ShadowData),
				.Flags = RHI::BufferUsage::Upload | RHI::BufferUsage::Constant,
			});
			RHI::Map(m_ShadowDataBuffer[i]);
		}
	}

	ShadowMapping::~ShadowMapping()
	{
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
			DestroyShader(m_ShadowMapShaders[cascade]);

		for (uint8 i = 0; i < RHI::NUM_BACK_BUFFERS; ++i)
			DestroyBuffer(m_ShadowDataBuffer[i]);
	}

	void ShadowMapping::BindShadowMap(RHI::Handle<RHI::Shader> shader)
	{
		RHI::Handle<RHI::Buffer> currentBuffer = m_ShadowDataBuffer[RHI::Device::Ptr->GetCurrentFrameIndex()];
		SetParameter(shader, "GShadowData", currentBuffer);
	}

	void ShadowMapping::Render(SceneRenderer* sceneRenderer)
	{
		if (UI::Globals::bDebugShadowMaps)
			DrawDebugWindow();

		CreateLightMatrices(&sceneRenderer->Camera, sceneRenderer->SceneInfo.SunDirection);

		// Shadow map
		RHI::BeginProfileEvent("Shadow Maps Pass");
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
		{
			RHI::Handle<RHI::Buffer> currentBuffer = m_ShadowDataBuffer[RHI::Device::Ptr->GetCurrentFrameIndex()];

			std::string profileName = std::format("Shadow Cascade {}", cascade);

			RHI::BeginProfileEvent(profileName.c_str());
			RHI::BindShader(m_ShadowMapShaders[cascade]);
			RHI::SetParameter(m_ShadowMapShaders[cascade], "GShadowData", currentBuffer);
			RHI::SetParameter(m_ShadowMapShaders[cascade], "cascadeIndex", cascade);
			sceneRenderer->BindSceneInfo(m_ShadowMapShaders[cascade]);
			for (const Scene* scene : sceneRenderer->GetScenes())
			{
				scene->IterateMeshes([&](const Mesh& mesh)
				{
					RHI::SetParameter(m_ShadowMapShaders[cascade], "instanceID", mesh.InstanceID);

					RHI::BindIndexBufferView(mesh.IndicesLocation);
					RHI::DrawIndexed((uint32)mesh.IndexCount);
				});
			}
			RHI::EndProfileEvent(profileName.c_str());
		}
		RHI::EndProfileEvent("Shadow Maps Pass");
}

	void ShadowMapping::DrawDebugWindow()
	{
		ImGui::Begin("Shadow Map Debug", &UI::Globals::bDebugShadowMaps);
		ImGui::Checkbox("Show Shadow Cascades", &UI::Globals::bShowShadowCascades);
		ImGui::SliderInt("Shadow Cascade", &UI::Globals::ShadowCascadeIndex, 0, SHADOWMAP_CASCADES - 1);
		ImGui::Image((ImTextureID)RHI::GetShaderRTTextureID(m_ShadowMapShaders[UI::Globals::ShadowCascadeIndex], 0), ImVec2(512, 512));
		ImGui::End();
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
			m_ShadowData.ShadowMap[cascade]		= GetTexture(GetShaderDepthTarget(m_ShadowMapShaders[cascade]))->SRVHandle.Index;

			lastSplitDist = cascadeSplits[cascade];
		}

		RHI::Handle<RHI::Buffer> currentBuffer = m_ShadowDataBuffer[RHI::Device::Ptr->GetCurrentFrameIndex()];
		memcpy(RHI::GetMappedData(currentBuffer), &m_ShadowData, sizeof(ShadowData));
	}
}

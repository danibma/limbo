#include "stdafx.h"
#include "shadowmapping.h"

#include "gfx/scene.h"
#include "gfx/rendercontext.h"
#include "gfx/shaderinterop.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/shadercompiler.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <format>

#include "gfx/psocache.h"

namespace limbo::Gfx
{
	namespace
	{
		bool bStabilizeCascades = true;
	}

	ShadowMapping::ShadowMapping()
		: RenderTechnique("Shadow Mapping")
	{
	}

	ShadowMapping::~ShadowMapping()
	{
		for (int i = 0; i < SHADOWMAP_CASCADES; ++i)
			RHI::DestroyTexture(m_ShadowMaps[i]);
	}

	bool ShadowMapping::Init()
	{
		for (int i = 0; i < SHADOWMAP_CASCADES; ++i)
			m_ShadowMaps[i] = RHI::CreateTexture(RHI::Tex2DDepth(SHADOWMAP_SIZES[i], SHADOWMAP_SIZES[i], 1.0f));

		return true;
	}

	bool ShadowMapping::ConditionalRender(RenderContext& context)
	{
		return context.CanRenderShadows();
	}

	void ShadowMapping::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		if (UIGlobals::bDebugShadowMaps)
			DrawDebugWindow();

		// Assign the current depth maps to the scene textures, so other techniques can use them
		for (int i = 0; i < SHADOWMAP_CASCADES; ++i)
			context.SceneTextures.ShadowMaps[i] = m_ShadowMaps[i];

		CreateLightMatrices(context);

		// Shadow map
		cmd.BeginProfileEvent("Shadow Maps Pass");
		cmd.SetPipelineState(PSOCache::Get(PipelineID::ShadowMapping));
		cmd.SetPrimitiveTopology();
		for (int cascade = 0; cascade < SHADOWMAP_CASCADES; ++cascade)
		{
			std::string profileName = std::format("Shadow Cascade {}", cascade);

			cmd.BeginProfileEvent(profileName.c_str());
			cmd.SetRenderTargets({}, m_ShadowMaps[cascade]);
			cmd.SetViewport(SHADOWMAP_SIZES[cascade], SHADOWMAP_SIZES[cascade]);

			cmd.ClearDepthTarget(m_ShadowMaps[cascade], 1.0f);

			cmd.BindConstants(2, 0, cascade);

			cmd.BindTempConstantBuffer(0, context.SceneInfo);
			cmd.BindTempConstantBuffer(1, context.ShadowMapData);

			for (const Scene* scene : context.GetScenes())
			{
				scene->IterateMeshes(TOnDrawMesh::CreateLambda([&](const Mesh& mesh)
				{
					cmd.BindConstants(2, 1, mesh.InstanceID);

					cmd.SetIndexBufferView(mesh.IndicesLocation);
					cmd.DrawIndexed((uint32)mesh.IndexCount);
				}));
			}
			cmd.EndProfileEvent(profileName.c_str());
		}
		cmd.EndProfileEvent("Shadow Maps Pass");
	}

	void ShadowMapping::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("Shadows"))
		{
			ImGui::Checkbox("Stabilize cascades", &bStabilizeCascades);
			ImGui::TreePop();
		}
	}

	void ShadowMapping::DrawDebugWindow()
	{
		ImGui::Begin("Shadow Map Debug", &UIGlobals::bDebugShadowMaps);
		ImGui::Checkbox("Show Shadow Cascades", &UIGlobals::bShowShadowCascades);
		ImGui::SliderInt("Shadow Cascade", &UIGlobals::ShadowCascadeIndex, 0, SHADOWMAP_CASCADES - 1);
		ImGui::Image((ImTextureID)RM_GET(m_ShadowMaps[UIGlobals::ShadowCascadeIndex])->TextureID(), ImVec2(512, 512));
		ImGui::End();
	}

	/*
		Calculate frustum split depths and matrices for the shadow map cascades
		Based on https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
	*/
	void ShadowMapping::CreateLightMatrices(RenderContext& context)
	{
		float cascadeSplits[SHADOWMAP_CASCADES];

		float nearClip = context.Camera.NearZ;
		float farClip = context.Camera.FarZ;
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
			float4x4 invCam = glm::inverse(context.Camera.ViewProj);
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

			glm::vec3 lightDir = glm::normalize(-context.SceneInfo.SunDirection);
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::orthoZO(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;

			if (bStabilizeCascades)
			{
				// Create the rounding matrix, by projecting the world-space origin and determining the fractional offset in texel space
				// https://github.com/TheRealMJP/Shadows/blob/master/Shadows/MeshRenderer.cpp#L1325
				float4 shadowOrigin = float4(0.0f, 0.0f, 0.0f, 1.0f);
				shadowOrigin = shadowMatrix * shadowOrigin;
				shadowOrigin *= SHADOWMAP_SIZES[cascade] / 2.0f;

				float4 roundedOrigin = glm::round(shadowOrigin);
				float4 roundOffset = roundedOrigin - shadowOrigin;
				roundOffset *=  2.0f / SHADOWMAP_SIZES[cascade];
				roundOffset.z = 0.0f;
				roundOffset.w = 0.0f;

				lightOrthoMatrix[3] += roundOffset;
				shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			}

			// Store split distance and matrix in cascade
			context.ShadowMapData.SplitDepth[cascade]    = (context.Camera.NearZ + splitDist * clipRange) * -1.0f;
			context.ShadowMapData.LightViewProj[cascade] = shadowMatrix;
			context.ShadowMapData.ShadowMap[cascade]	 = RM_GET(m_ShadowMaps[cascade])->SRV();

			lastSplitDist = cascadeSplits[cascade];
		}
	}
}

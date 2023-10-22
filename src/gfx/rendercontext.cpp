﻿#include "stdafx.h"
#include "rendercontext.h"

#include "profiler.h"
#include "psocache.h"
#include "scene.h"
#include "ui.h"
#include "core/jobsystem.h"
#include "rhi/device.h"
#include "techniques/pathtracing/pathtracing.h"
#include "techniques/rtao/rtao.h"
#include "techniques/shadowmapping/shadowmapping.h"
#include "techniques/ssao/ssao.h"
#include "rhi/resourcemanager.h"
#include "rhi/commandcontext.h"

namespace limbo::Gfx
{
	RenderContext::RenderContext(Core::Window* window)
		: Window(window)
		, Camera(CreateCamera(window, float3(0.0f, 1.0f, 4.0f), float3(0.0f, 0.0f, -1.0f)))
		, Light({.Position = float3(0.0f, 0.5f, 0.0f), .Color = float3(1, 0.45f, 0) })
		, Sun({ 0.5f, 12.0f, 2.0f })
	{
		Core::Timer initTimer;

		RHI::OnResizedSwapchain.AddRaw(&Camera, &FPSCamera::OnResize);
		RHI::OnResizedSwapchain.AddRaw(this, &RenderContext::UpdateSceneTextures);

		RenderSize = { RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight() };
		CreateSceneTextures(RenderSize.x, RenderSize.y);

		const auto& rendererNames = Gfx::Renderer::GetNames();
		CurrentRendererString = *rendererNames.cbegin();
		CurrentRenderer = Renderer::Create(CurrentRendererString);
		CurrentRenderTechniques = CurrentRenderer->SetupRenderTechniques();
		for (auto& i : CurrentRenderTechniques)
		{
			check(i->Init());
			CurrentRenderOptions.Options.merge(i->GetOptions());
		}

		EnvironmentMaps = {
			"assets/environment/a_rotes_rathaus_4k.hdr",
			"assets/environment/footprint_court.hdr",
			"assets/environment/helipad.hdr",
			"assets/environment/kiara_1_dawn_4k.hdr",
			"assets/environment/pink_sunrise_4k.hdr",
			"assets/environment/spree_bank_4k.hdr",
			"assets/environment/the_sky_is_on_fire_4k.hdr",
			"assets/environment/venice_dawn_1_4k.hdr"
		};

		//LoadNewScene("assets/models/cornell_box.glb");
		//LoadNewScene("assets/models/vulkanscene_shadow.gltf");
		LoadNewScene("assets/models/Sponza/Sponza.gltf");

		LB_LOG("Took %.2fs to initialize the Scene Renderer", initTimer.ElapsedSeconds());
	}

	RenderContext::~RenderContext()
	{
		DestroySceneTextures();

		RHI::DestroyTexture(SceneTextures.EnvironmentCubemap);
		RHI::DestroyTexture(SceneTextures.IrradianceMap);
		RHI::DestroyTexture(SceneTextures.PrefilterMap);
		RHI::DestroyTexture(SceneTextures.BRDFLUTMap);

		DestroyBuffer(m_ScenesMaterials);
		DestroyBuffer(m_SceneInstances);

		for (Scene* scene : m_Scenes)
			DestroyScene(scene);
	}

	std::vector<RHI::TextureHandle> RenderContext::GetGBufferTextures() const
	{
		return {
			SceneTextures.GBufferRenderTargetA,
			SceneTextures.GBufferRenderTargetB,
			SceneTextures.GBufferRenderTargetC,
			SceneTextures.GBufferRenderTargetD,
			SceneTextures.GBufferRenderTargetE,
			SceneTextures.GBufferRenderTargetF,
		};
	}

	void RenderContext::Render(float dt)
	{
		if (bUpdateRenderer)
		{
			UpdateRenderer();
			bUpdateRenderer = false;
		}

		RHI::CommandContext* cmd = RHI::CommandContext::GetCommandContext();

		UI::Render(*this, dt);

		PROFILE_SCOPE(cmd, "Render");

		if (bNeedsEnvMapChange)
		{
			cmd->BeginEvent("Loading Environment Map");
			LoadEnvironmentMap(cmd, EnvironmentMaps[Tweaks.SelectedEnvMapIdx]);
			bNeedsEnvMapChange = false;
			cmd->EndEvent();
		}

		SceneAccelerationStructure.Build(cmd, m_Scenes);

		UpdateSceneInfo();

		for (auto& i : CurrentRenderTechniques)
		{
			if (i->ConditionalRender(*this))
				i->Render(*cmd, *this);
		}

		// present
		{
			PROFILE_GPU_SCOPE(cmd, "Present");
			RHI::Present(Tweaks.bEnableVSync);
		}
	}

	void RenderContext::LoadNewScene(const char* path)
	{
		m_Scenes.emplace_back(Scene::Load(path));
		UploadScenesToGPU();
	}

	void RenderContext::UploadScenesToGPU()
	{
		auto appendArrays = [](auto& outArray, const auto& fromArray)
		{
			outArray.reserve(outArray.capacity() + fromArray.size());
			outArray.insert(outArray.end(), fromArray.begin(), fromArray.end());
		};

		auto uploadArrayToGPU = [](RHI::BufferHandle& buffer, const auto& arr, const char* debugName)
		{
			if (buffer.IsValid())
				DestroyBuffer(buffer);

			buffer = RHI::CreateBuffer({
				.DebugName = debugName,
				.NumElements = (uint32)arr.size(),
				.ByteStride = sizeof(arr[0]),
				.ByteSize = arr.size() * sizeof(arr[0]),
				.Flags = RHI::BufferUsage::Structured | RHI::BufferUsage::ShaderResourceView,
				.InitialData = arr.data(),
			});
		};

		std::vector<Material> materials;
		std::vector<Instance> instances;
		uint32 instanceID = 0;
		for (Scene* scene : m_Scenes)
		{
			scene->IterateMeshesNoConst(TOnDrawMeshNoConst::CreateLambda([&](Mesh& mesh)
			{
				mesh.InstanceID = instanceID;
				Instance& instance = instances.emplace_back();
				instance.LocalTransform  = mesh.Transform;
				instance.Material		 = (uint32)materials.size() + mesh.LocalMaterialIndex;
				instance.PositionsOffset = mesh.PositionsLocation.Offset;
				instance.NormalsOffset   = mesh.NormalsLocation.Offset;
				instance.TexCoordsOffset = mesh.TexCoordsLocation.Offset;
				instance.IndicesOffset	 = mesh.IndicesLocation.Offset;
				instance.BufferIndex	 = scene->GetGeometryBuffer()->CBVHandle.Index;
				instanceID++;
			}));

			appendArrays(materials, scene->Materials);
		}

		uploadArrayToGPU(m_ScenesMaterials, materials, "ScenesMaterials");
		uploadArrayToGPU(m_SceneInstances,  instances, "SceneInstances");

		SceneInfo.MaterialsBufferIndex = RM_GET(m_ScenesMaterials)->CBVHandle.Index;
		SceneInfo.InstancesBufferIndex = RM_GET(m_SceneInstances)->CBVHandle.Index;
	}

	void RenderContext::UpdateSceneInfo()
	{
		SceneInfo.bSunCastsShadows		= Tweaks.bSunCastsShadows;
		SceneInfo.SunDirection			= float4(Sun.Direction, 1.0f);
		SceneInfo.bShowShadowCascades	= UI::Globals::bShowShadowCascades;

		SceneInfo.PrevView				= SceneInfo.View;

		SceneInfo.View					= Camera.View;
		SceneInfo.InvView				= glm::inverse(Camera.View);
		SceneInfo.Projection			= Camera.RevProj;
		SceneInfo.InvProjection			= glm::inverse(Camera.RevProj);
		SceneInfo.ViewProjection		= Camera.ViewRevProj;
		SceneInfo.CameraPos				= Camera.Eye;
		SceneInfo.SkyIndex				= RM_GET(SceneTextures.EnvironmentCubemap)->SRV();
		SceneInfo.SceneViewToRender		= Tweaks.CurrentSceneView;
		SceneInfo.FrameIndex++;
	}

	void RenderContext::UpdateRenderer()
	{
		CurrentRenderTechniques.clear();
		CurrentRenderOptions.Options.clear();

		CurrentRenderer = Renderer::Create(CurrentRendererString);
		CurrentRenderTechniques = CurrentRenderer->SetupRenderTechniques();
		for (auto& i : CurrentRenderTechniques)
		{
			check(i->Init());
			CurrentRenderOptions.Options.merge(i->GetOptions());
		}
	}

	void RenderContext::CreateSceneTextures(uint32 width, uint32 height)
	{
		float clearValue[4] = { 0 };
		SceneTextures.PreCompositeSceneTexture = RHI::CreateTexture({
			.Width = width,
			.Height = height,
			.DebugName = "PreCompositeSceneTexture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource | RHI::TextureUsage::RenderTarget,
			.ClearValue = {
				.Format = RHI::D3DFormat(RHI::GetSwapchainFormat()),
				.Color = clearValue[0]
			},
			.Format = RHI::GetSwapchainFormat(),
			.Type = RHI::TextureType::Texture2D,
		});

		SceneTextures.AOTexture = RHI::CreateTexture({
			.Width = width,
			.Height = height,
			.DebugName = "AO Texture",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::R8_UNORM,
		});

		SceneTextures.GBufferRenderTargetA = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetA"));
		SceneTextures.GBufferRenderTargetB = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetB"));
		SceneTextures.GBufferRenderTargetC = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetC"));
		SceneTextures.GBufferRenderTargetD = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetD"));
		SceneTextures.GBufferRenderTargetE = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetE"));
		SceneTextures.GBufferRenderTargetF = RHI::CreateTexture(RHI::Tex2DRenderTarget(width, height, RHI::Format::RGBA16_SFLOAT, "GBufferRenderTargetF"));
		SceneTextures.GBufferDepthTarget = RHI::CreateTexture(RHI::Tex2DDepth(width, height, 0.0f, "GBuffer Depth"));
	}

	void RenderContext::UpdateSceneTextures(uint32 width, uint32 height)
	{
		DestroySceneTextures();
		CreateSceneTextures(width, height);
	}

	void RenderContext::DestroySceneTextures()
	{
		RHI::DestroyTexture(SceneTextures.PreCompositeSceneTexture);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetA);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetB);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetC);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetD);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetE);
		RHI::DestroyTexture(SceneTextures.GBufferRenderTargetF);
		RHI::DestroyTexture(SceneTextures.GBufferDepthTarget);
		RHI::DestroyTexture(SceneTextures.AOTexture);
	}

	void RenderContext::ClearScenes()
	{
		for (Scene* scene : m_Scenes)
			DestroyScene(scene);
		m_Scenes.clear();
	}

	bool RenderContext::HasScenes() const
	{
		return m_Scenes.size() > 0;
	}

	const std::vector<Scene*>& RenderContext::GetScenes() const
	{
		return m_Scenes;
	}

	void RenderContext::LoadEnvironmentMap(RHI::CommandContext* cmd, const char* path)
	{
		if (SceneTextures.EnvironmentCubemap.IsValid())
			RHI::DestroyTexture(SceneTextures.EnvironmentCubemap);
		if (SceneTextures.IrradianceMap.IsValid())
			RHI::DestroyTexture(SceneTextures.IrradianceMap);
		if (SceneTextures.PrefilterMap.IsValid())
			RHI::DestroyTexture(SceneTextures.PrefilterMap);
		if (SceneTextures.BRDFLUTMap.IsValid())
			RHI::DestroyTexture(SceneTextures.BRDFLUTMap);

		uint2 equirectangularTextureSize = { 512, 512 };
		SceneTextures.EnvironmentCubemap = RHI::CreateTexture({
			.Width = equirectangularTextureSize.x,
			.Height = equirectangularTextureSize.y,
			.DebugName = "Environment Cubemap",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA16_SFLOAT,
			.Type = RHI::TextureType::TextureCube,
		});

		// transform an equirectangular map into a cubemap
		{
			RHI::TextureHandle equirectangularTexture = RHI::CreateTextureFromFile(path, "Equirectangular Texture");

			cmd->BeginEvent("EquirectToCubemap");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::EquirectToCubemap));
			cmd->BindConstants(1, 0, RM_GET(equirectangularTexture)->SRV());

			RHI::DescriptorHandle uavTexture[] =
			{
				RM_GET(SceneTextures.EnvironmentCubemap)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(2, uavTexture, 1);
			cmd->Dispatch(equirectangularTextureSize.x / 8, equirectangularTextureSize.y / 8, 6);
			cmd->EndEvent();

			RHI::DestroyTexture(equirectangularTexture);
		}

		// irradiance map
		{
			uint2 irradianceSize = { 32, 32 };
			SceneTextures.IrradianceMap = RHI::CreateTexture({
				.Width = irradianceSize.x,
				.Height = irradianceSize.y,
				.DebugName = "Irradiance Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RGBA16_SFLOAT,
				.Type = RHI::TextureType::TextureCube,
			});

			cmd->BeginEvent("DrawIrradianceMap");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::DrawIrradianceMap));
			cmd->BindConstants(0, 0, RM_GET(SceneTextures.EnvironmentCubemap)->SRV());

			RHI::DescriptorHandle uavTexture[] =
			{
				RM_GET(SceneTextures.IrradianceMap)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(2, uavTexture, 1);
			cmd->Dispatch(irradianceSize.x / 8, irradianceSize.y / 8, 6);
			cmd->EndEvent();
		}

		// Pre filter env map
		{
			uint2 prefilterSize = { 128, 128 };
			uint16 prefilterMipLevels = 5;

			SceneTextures.PrefilterMap = RHI::CreateTexture({
				.Width = prefilterSize.x,
				.Height = prefilterSize.y,
				.MipLevels = prefilterMipLevels,
				.DebugName = "PreFilter Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RGBA16_SFLOAT,
				.Type = RHI::TextureType::TextureCube,
			});

			cmd->BeginEvent("PreFilterEnvMap");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::PreFilterEnvMap));
			cmd->BindConstants(1, 0, RM_GET(SceneTextures.EnvironmentCubemap)->SRV());

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 8);

				RHI::DescriptorHandle uavTexture[] =
				{
					RM_GET(SceneTextures.PrefilterMap)->UAVHandle[level]
				};
				cmd->BindTempDescriptorTable(0, uavTexture, 1);
				cmd->BindConstants(2, 0, level * deltaRoughness);
				cmd->Dispatch(numGroups, numGroups, 6);
			}
			cmd->EndEvent();
		}

		// Compute BRDF LUT map
		{
			uint2 brdfLUTMapSize = { 512, 512 };
			SceneTextures.BRDFLUTMap = RHI::CreateTexture({
				.Width = brdfLUTMapSize.x,
				.Height = brdfLUTMapSize.y,
				.DebugName = "BRDF LUT Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RG16_SFLOAT,
				.Type = RHI::TextureType::Texture2D,
			});

			cmd->BeginEvent("ComputeBRDFLUT");
			cmd->SetPipelineState(PSOCache::Get(PipelineID::ComputeBRDFLUT));

			RHI::DescriptorHandle uavTexture[] =
			{
				RM_GET(SceneTextures.BRDFLUTMap)->UAVHandle[0]
			};
			cmd->BindTempDescriptorTable(0, uavTexture, 1);
			cmd->Dispatch(brdfLUTMapSize.x / 8, brdfLUTMapSize.y / 8, 6);
			cmd->EndEvent();
		}
	}
}

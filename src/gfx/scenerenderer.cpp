#include "stdafx.h"
#include "scenerenderer.h"

#include <filesystem>

#include "profiler.h"
#include "scene.h"
#include "uirenderer.h"
#include "core/jobsystem.h"
#include "rhi/device.h"
#include "techniques/pathtracing.h"
#include "techniques/rtao.h"
#include "techniques/shadowmapping.h"
#include "techniques/ssao.h"

namespace limbo::Gfx
{
	SceneRenderer::SceneRenderer(Core::Window* window)
		: Window(window)
		, Camera(CreateCamera(float3(0.0f, 1.0f, 4.0f), float3(0.0f, 0.0f, -1.0f)))
		, Light({.Position = float3(0.0f, 0.5f, 0.0f), .Color = float3(1, 0.45f, 0) })
		, Sun({ 0.5f, 12.0f, 2.0f })
	{
		RHI::Device::Ptr->OnResizedSwapchain.AddRaw(&Camera, &FPSCamera::OnResize);

		const char* env_maps_path = "assets/environment";
		for (const auto& entry : std::filesystem::directory_iterator(env_maps_path))
			EnvironmentMaps.emplace_back(entry.path());

		for(uint8 i = 0; i < RHI::NUM_BACK_BUFFERS; ++i)
		{
			std::string debugName = std::format("SceneInfoBuffer[{}]", i);
			m_SceneInfoBuffers[i] = RHI::CreateBuffer({
				.DebugName = debugName.c_str(),
				.ByteSize = sizeof(SceneInfo),
				.Flags = RHI::BufferUsage::Upload | RHI::BufferUsage::Constant,
			});
			RHI::Map(m_SceneInfoBuffers[i]);
		}

		Core::JobSystem::Execute([this]()
		{
			m_SSAO = std::make_unique<SSAO>();
			m_RTAO = std::make_unique<RTAO>();
			m_PathTracing = std::make_unique<PathTracing>();
			m_ShadowMapping = std::make_unique<ShadowMapping>();
		});

		//LoadNewScene("assets/models/cornell_box.glb");
		//LoadNewScene("assets/models/vulkanscene_shadow.gltf");
		LoadNewScene("assets/models/Sponza/Sponza.gltf");

		// Deferred shader
		m_DeferredShadingShader = RHI::CreateShader({
			.ProgramName = "deferredshading",
			.RTFormats = {
				{ RHI::Format::RGBA16_SFLOAT, "PixelPosition"},
				{ RHI::Format::RGBA16_SFLOAT, "WorldPosition"},
				{ RHI::Format::RGBA16_SFLOAT, "Albedo"},
				{ RHI::Format::RGBA16_SFLOAT, "Normal" },
				{ RHI::Format::RGBA16_SFLOAT, "RoughnessMetallicAO" },
				{ RHI::Format::RGBA16_SFLOAT, "Emissive" },
			},
			.DepthFormat = { RHI::Format::D32_SFLOAT },
			.Type = RHI::ShaderType::Graphics
		});

		// Skybox
		m_SkyboxCube = Scene::Load("assets/models/skybox.glb");
		m_SkyboxShader = RHI::CreateShader({
			.ProgramName = "sky",
			.RTFormats = {
				{ RHI::Format::RGBA16_SFLOAT, "PBR RT" }
			},
			.DepthFormat = { RHI::Format::D32_SFLOAT, "PBR Depth" },
			.Type = RHI::ShaderType::Graphics
		});

		// PBR
		m_PBRShader = RHI::CreateShader({
			.ProgramName = "pbrlighting",
			.RTFormats = {
				{
					.RTFormat = RHI::Format::RGBA16_SFLOAT,
					.RTTexture = RHI::GetShaderRT(m_SkyboxShader, 0),
					.LoadRenderPassOp = RHI::RenderPassOp::Load
				}
			},
			.DepthFormat = {
				.RTFormat = RHI::Format::D32_SFLOAT,
				.RTTexture = RHI::GetShaderDepthTarget(m_SkyboxShader),
				.LoadRenderPassOp = RHI::RenderPassOp::Load
			},
			.Type = RHI::ShaderType::Graphics
		});

		// Composite shader
		m_CompositeShader = RHI::CreateShader({
			.ProgramName = "scenecomposite",
			.Type = RHI::ShaderType::Graphics
		});

		Core::JobSystem::WaitIdle();
	}

	SceneRenderer::~SceneRenderer()
	{
		DestroyTexture(m_EnvironmentCubemap);
		DestroyTexture(m_IrradianceMap);
		DestroyTexture(m_PrefilterMap);
		DestroyTexture(m_BRDFLUTMap);

		DestroyShader(m_PBRShader);
		DestroyShader(m_SkyboxShader);
		DestroyShader(m_DeferredShadingShader);
		DestroyShader(m_CompositeShader);

		for (uint8 i = 0; i < RHI::NUM_BACK_BUFFERS; ++i)
			DestroyBuffer(m_SceneInfoBuffers[i]);

		DestroyBuffer(m_ScenesMaterials);
		DestroyBuffer(m_SceneInstances);

		DestroyScene(m_SkyboxCube);
		for (Scene* scene : m_Scenes)
			DestroyScene(scene);
	}

	void SceneRenderer::RenderGeometryPass()
	{
		RHI::BeginProfileEvent("Geometry Pass");
		RHI::BindShader(m_DeferredShadingShader);
		BindSceneInfo(m_DeferredShadingShader);
		for (Scene* scene : m_Scenes)
		{
			scene->IterateMeshes([&](const Mesh& mesh)
			{
				RHI::SetParameter(m_DeferredShadingShader, "instanceID", mesh.InstanceID);

				RHI::BindIndexBufferView(mesh.IndicesLocation);
				RHI::DrawIndexed((uint32)mesh.IndexCount);
			});
		}
		RHI::EndProfileEvent("Geometry Pass");
	}

	void SceneRenderer::RenderAmbientOcclusionPass()
	{
		PROFILE_SCOPE(RHI::GetCommandContext(RHI::ContextType::Direct), "Ambient Occlusion");
		if (Tweaks.CurrentAOTechnique == (int)AmbientOcclusion::SSAO)
			m_SSAO->Render(this, GetShaderRT(m_DeferredShadingShader, 0), GetShaderDepthTarget(m_DeferredShadingShader));
		else if (Tweaks.CurrentAOTechnique == (int)AmbientOcclusion::RTAO)
			m_RTAO->Render(this, &m_SceneAS, GetShaderRT(m_DeferredShadingShader, 1), GetShaderRT(m_DeferredShadingShader, 3));
	}

	void SceneRenderer::RenderSkybox()
	{
		RHI::BeginProfileEvent("Render Skybox");
		RHI::BindShader(m_SkyboxShader);
		BindSceneInfo(m_SkyboxShader);
		RHI::SetParameter(m_SkyboxShader, "g_EnvironmentCube", m_EnvironmentCubemap);
		m_SkyboxCube->IterateMeshes([&](const Mesh& mesh)
		{
			RHI::BindVertexBufferView(mesh.PositionsLocation);
			RHI::BindIndexBufferView(mesh.IndicesLocation);
			RHI::DrawIndexed((uint32)mesh.IndexCount);
		});
		RHI::EndProfileEvent("Render Skybox");
	}

	void SceneRenderer::RenderLightingPass()
	{
		RHI::BeginProfileEvent("Lighting");
		RHI::BindShader(m_PBRShader);
		BindSceneInfo(m_PBRShader);
		RHI::SetParameter(m_PBRShader, "bEnableAO", Tweaks.CurrentAOTechnique);
		m_ShadowMapping->BindShadowMap(m_PBRShader);
		// PBR scene info
		RHI::SetParameter(m_PBRShader, "lightPos", Light.Position);
		RHI::SetParameter(m_PBRShader, "lightColor", Light.Color);
		// Bind deferred shading render targets
		RHI::SetParameter(m_PBRShader, "g_PixelPosition", m_DeferredShadingShader, 0);
		RHI::SetParameter(m_PBRShader, "g_WorldPosition", m_DeferredShadingShader, 1);
		RHI::SetParameter(m_PBRShader, "g_Albedo", m_DeferredShadingShader, 2);
		RHI::SetParameter(m_PBRShader, "g_Normal", m_DeferredShadingShader, 3);
		RHI::SetParameter(m_PBRShader, "g_RoughnessMetallicAO", m_DeferredShadingShader, 4);
		RHI::SetParameter(m_PBRShader, "g_Emissive", m_DeferredShadingShader, 5);

		if (Tweaks.CurrentAOTechnique == (int)AmbientOcclusion::RTAO)
			RHI::SetParameter(m_PBRShader, "g_AmbientOcclusion", m_RTAO->GetFinalTexture());
		else
			RHI::SetParameter(m_PBRShader, "g_AmbientOcclusion", m_SSAO->GetBlurredTexture());

		// Bind irradiance map
		RHI::SetParameter(m_PBRShader, "g_IrradianceMap", m_IrradianceMap);
		RHI::SetParameter(m_PBRShader, "g_PrefilterMap", m_PrefilterMap);
		RHI::SetParameter(m_PBRShader, "g_LUT", m_BRDFLUTMap);
		RHI::Draw(6);
		RHI::EndProfileEvent("Lighting");
	}

	void SceneRenderer::RenderSceneCompositePass()
	{
		RHI::BeginProfileEvent("Scene Composite");
		RHI::BindShader(m_CompositeShader);
		RHI::SetParameter(m_CompositeShader, "g_TonemapMode", Tweaks.CurrentTonemap);
		RHI::SetParameter(m_CompositeShader, "g_sceneTexture", m_SceneTexture);
		RHI::Draw(6);
		RHI::EndProfileEvent("Scene Composite");
	}

	void SceneRenderer::Render(float dt)
	{
		PROFILE_SCOPE(RHI::GetCommandContext(), "Render");

		if (bNeedsEnvMapChange)
		{
			RHI::BeginEvent("Loading Environment Map");
			LoadEnvironmentMap(EnvironmentMaps[Tweaks.SelectedEnvMapIdx].string().c_str());
			bNeedsEnvMapChange = false;
			RHI::EndEvent();
		}

		m_SceneAS.Build(m_Scenes);

		UpdateSceneInfo();

		if (Tweaks.CurrentRenderPath == (int)RenderPath::Deferred)
		{
			if (SceneInfo.bSunCastsShadows)
				m_ShadowMapping->Render(this);

			RenderGeometryPass();
			RenderAmbientOcclusionPass();

			RenderSkybox();

			RenderLightingPass();

			m_SceneTexture = GetShaderRT(m_PBRShader, 0);
		}
		else if (Tweaks.CurrentRenderPath == (int)RenderPath::PathTracing)
		{
			m_PathTracing->Render(this, &m_SceneAS, Camera);
			m_SceneTexture = m_PathTracing->GetFinalTexture();
		}

		// render scene composite
		RenderSceneCompositePass();

		// present
		{
			PROFILE_GPU_SCOPE(RHI::GetCommandContext(RHI::ContextType::Direct), "Present");
			RHI::Present(Tweaks.bEnableVSync);
		}
	}

	void SceneRenderer::LoadNewScene(const char* path)
	{
		m_Scenes.emplace_back(Scene::Load(path));
		UploadScenesToGPU();
	}

	void SceneRenderer::UploadScenesToGPU()
	{
		auto appendArrays = [](auto& outArray, const auto& fromArray)
		{
			outArray.reserve(outArray.capacity() + fromArray.size());
			outArray.insert(outArray.end(), fromArray.begin(), fromArray.end());
		};

		auto uploadArrayToGPU = [](RHI::Handle<RHI::Buffer>& buffer, const auto& arr, const char* debugName)
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
			scene->IterateMeshesNoConst([&](Mesh& mesh)
			{
				mesh.InstanceID = instanceID;
				Instance& instance = instances.emplace_back();
				instance.LocalTransform  = mesh.Transform;
				instance.Material		 = (uint32)materials.size() + mesh.LocalMaterialIndex;
				instance.PositionsOffset = mesh.PositionsLocation.Offset;
				instance.NormalsOffset   = mesh.NormalsLocation.Offset;
				instance.TexCoordsOffset = mesh.TexCoordsLocation.Offset;
				instance.IndicesOffset	 = mesh.IndicesLocation.Offset;
				instance.BufferIndex	 = scene->GetGeometryBuffer()->BasicHandle.Index;
				instanceID++;
			});

			appendArrays(materials, scene->Materials);
		}

		uploadArrayToGPU(m_ScenesMaterials, materials, "ScenesMaterials");
		uploadArrayToGPU(m_SceneInstances,  instances, "SceneInstances");

		SceneInfo.MaterialsBufferIndex = GetBuffer(m_ScenesMaterials)->BasicHandle.Index;
		SceneInfo.InstancesBufferIndex = GetBuffer(m_SceneInstances)->BasicHandle.Index;
	}

	void SceneRenderer::UpdateSceneInfo()
	{
		SceneInfo.bSunCastsShadows		= Tweaks.bSunCastsShadows;
		SceneInfo.SunDirection			= float4(Sun.Direction, 1.0f);
		SceneInfo.bShowShadowCascades	= UI::Globals::bShowShadowCascades;

		SceneInfo.PrevView				= SceneInfo.View;

		SceneInfo.View					= Camera.View;
		SceneInfo.InvView				= glm::inverse(Camera.View);
		SceneInfo.Projection			= Camera.Proj;
		SceneInfo.InvProjection			= glm::inverse(Camera.Proj);
		SceneInfo.ViewProjection		= Camera.ViewProj;
		SceneInfo.CameraPos				= Camera.Eye;
		SceneInfo.SkyIndex				= GetTexture(m_EnvironmentCubemap)->SRVHandle.Index;
		SceneInfo.SceneViewToRender		= Tweaks.CurrentSceneView;
		SceneInfo.FrameIndex++;

		RHI::Handle<RHI::Buffer> currentBuffer = m_SceneInfoBuffers[RHI::Device::Ptr->GetCurrentFrameIndex()];
		memcpy(RHI::GetMappedData(currentBuffer), &SceneInfo, sizeof(SceneInfo));
	}

	void SceneRenderer::ClearScenes()
	{
		for (Scene* scene : m_Scenes)
			DestroyScene(scene);
		m_Scenes.clear();
	}

	bool SceneRenderer::HasScenes() const
	{
		return m_Scenes.size() > 0;
	}

	const std::vector<Scene*>& SceneRenderer::GetScenes() const
	{
		return m_Scenes;
	}

	void SceneRenderer::BindSceneInfo(RHI::Handle<RHI::Shader> shaderToBind)
	{
		RHI::Handle<RHI::Buffer> currentBuffer = m_SceneInfoBuffers[RHI::Device::Ptr->GetCurrentFrameIndex()];
		SetParameter(shaderToBind, "GSceneInfo", currentBuffer);
	}

	void SceneRenderer::LoadEnvironmentMap(const char* path)
	{
		if (m_EnvironmentCubemap.IsValid())
			DestroyTexture(m_EnvironmentCubemap);
		if (m_IrradianceMap.IsValid())
			DestroyTexture(m_IrradianceMap);
		if (m_PrefilterMap.IsValid())
			DestroyTexture(m_PrefilterMap);
		if (m_BRDFLUTMap.IsValid())
			DestroyTexture(m_BRDFLUTMap);

		uint2 equirectangularTextureSize = { 512, 512 };
		m_EnvironmentCubemap = RHI::CreateTexture({
			.Width = equirectangularTextureSize.x,
			.Height = equirectangularTextureSize.y,
			.DebugName = "Environment Cubemap",
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = RHI::Format::RGBA16_SFLOAT,
			.Type = RHI::TextureType::TextureCube,
		});

		// transform an equirectangular map into a cubemap
		{
			RHI::BeginEvent("EquirectToCubemap");
			RHI::Handle<RHI::Texture> equirectangularTexture = RHI::CreateTextureFromFile(path, "Equirectangular Texture");
			RHI::Handle<RHI::Shader> equirectToCubemap = RHI::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "EquirectToCubemap",
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(equirectToCubemap);
			RHI::SetParameter(equirectToCubemap, "g_EnvironmentEquirectangular", equirectangularTexture);
			RHI::SetParameter(equirectToCubemap, "g_OutEnvironmentCubemap", m_EnvironmentCubemap);
			RHI::Dispatch(equirectangularTextureSize.x / 8, equirectangularTextureSize.y / 8, 6);

			RHI::DestroyShader(equirectToCubemap);
			RHI::DestroyTexture(equirectangularTexture);
			RHI::EndEvent();
		}

		// irradiance map
		{
			RHI::BeginEvent("DrawIrradianceMap");
			uint2 irradianceSize = { 32, 32 };
			m_IrradianceMap = RHI::CreateTexture({
				.Width = irradianceSize.x,
				.Height = irradianceSize.y,
				.DebugName = "Irradiance Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RGBA16_SFLOAT,
				.Type = RHI::TextureType::TextureCube,
			});

			RHI::Handle<RHI::Shader> irradianceShader = RHI::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "DrawIrradianceMap",
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(irradianceShader);
			RHI::SetParameter(irradianceShader, "g_EnvironmentCubemap", m_EnvironmentCubemap);
			RHI::SetParameter(irradianceShader, "g_IrradianceMap", m_IrradianceMap);
			RHI::Dispatch(irradianceSize.x / 8, irradianceSize.y / 8, 6);

			RHI::DestroyShader(irradianceShader);
			RHI::EndEvent();
		}

		// Pre filter env map
		{
			RHI::BeginEvent("PreFilterEnvMap");
			uint2 prefilterSize = { 128, 128 };
			uint16 prefilterMipLevels = 5;

			m_PrefilterMap = RHI::CreateTexture({
				.Width = prefilterSize.x,
				.Height = prefilterSize.y,
				.MipLevels = prefilterMipLevels,
				.DebugName = "PreFilter Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RGBA16_SFLOAT,
				.Type = RHI::TextureType::TextureCube,
			});

			RHI::Handle<RHI::Shader> prefilterShader = RHI::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "PreFilterEnvMap",
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(prefilterShader);
			RHI::SetParameter(prefilterShader, "g_EnvironmentCubemap", m_EnvironmentCubemap);

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 8);

				RHI::SetParameter(prefilterShader, "g_PreFilteredMap", m_PrefilterMap, level);
				RHI::SetParameter(prefilterShader, "roughness", level * deltaRoughness);
				RHI::Dispatch(numGroups, numGroups, 6);
			}

			RHI::DestroyShader(prefilterShader);
			RHI::EndEvent();
		}

		// Compute BRDF LUT map
		{
			RHI::BeginEvent("ComputeBRDFLUT");
			uint2 brdfLUTMapSize = { 512, 512 };
			m_BRDFLUTMap = RHI::CreateTexture({
				.Width = brdfLUTMapSize.x,
				.Height = brdfLUTMapSize.y,
				.DebugName = "BRDF LUT Map",
				.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
				.Format = RHI::Format::RG16_SFLOAT,
				.Type = RHI::TextureType::Texture2D,
			});

			RHI::Handle<RHI::Shader> brdfLUTShader = RHI::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "ComputeBRDFLUT",
				.Type = RHI::ShaderType::Compute
			});
			RHI::BindShader(brdfLUTShader);
			RHI::SetParameter(brdfLUTShader, "LUT", m_BRDFLUTMap);
			RHI::Dispatch(brdfLUTMapSize.x / 8, brdfLUTMapSize.y / 8, 6);

			RHI::DestroyShader(brdfLUTShader);
			RHI::EndEvent();
		}
	}
}

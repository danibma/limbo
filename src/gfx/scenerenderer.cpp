﻿#include "stdafx.h"
#include "scenerenderer.h"

#include <filesystem>

#include "scene.h"
#include "rhi/device.h"
#include "techniques/pathtracing.h"
#include "techniques/ssao.h"

namespace limbo::Gfx
{
	SceneRenderer::SceneRenderer(Core::Window* window)
		: Window(window)
		, Camera(CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f)))
		, Light({.Position = float3(0.0f, 1.0f, 0.0f), .Color = float3(1, 0.45f, 0) })
		, FrameIndex(0)
	{
		const char* env_maps_path = "assets/environment";
		for (const auto& entry : std::filesystem::directory_iterator(env_maps_path))
			EnvironmentMaps.emplace_back(entry.path());

		for(uint8 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			std::string debugName = std::format("SceneInfoBuffer[{}]", i);
			m_SceneInfoBuffers[i] = CreateBuffer({
				.DebugName = debugName.c_str(),
				.ByteSize = sizeof(SceneInfo),
				.Usage = BufferUsage::Upload,
				.bCreateView = true
			});
			Gfx::Map(m_SceneInfoBuffers[i]);
		}

		LoadNewScene("assets/models/Sponza/Sponza.gltf");

		// Deferred shader
		m_DeferredShadingShader = Gfx::CreateShader({
			.ProgramName = "deferredshading",
			.RTFormats = {
				{ Gfx::Format::RGBA16_SFLOAT, "PixelPosition"},
				{ Gfx::Format::RGBA16_SFLOAT, "WorldPosition"},
				{ Gfx::Format::RGBA16_SFLOAT, "Albedo"},
				{ Gfx::Format::RGBA16_SFLOAT, "Normal" },
				{ Gfx::Format::RGBA16_SFLOAT, "RoughnessMetallic" },
				{ Gfx::Format::RGBA16_SFLOAT, "Emissive" },
				{ Gfx::Format::R16_SFLOAT,    "AmbientOcclusion" },
			},
			.DepthFormat = { Gfx::Format::D32_SFLOAT },
			.Type = Gfx::ShaderType::Graphics
		});

		// Skybox
		m_SkyboxCube = Gfx::Scene::Load("assets/models/skybox.glb");
		m_SkyboxShader = Gfx::CreateShader({
			.ProgramName = "sky",
			.RTFormats = {
				{ Gfx::Format::RGBA16_SFLOAT, "PBR RT" }
			},
			.DepthFormat = { Gfx::Format::D32_SFLOAT, "PBR Depth" },
			.Type = Gfx::ShaderType::Graphics
		});

		// PBR
		m_PBRShader = Gfx::CreateShader({
			.ProgramName = "pbrlighting",
			.RTFormats = {
				{
					.RTFormat = Gfx::Format::RGBA16_SFLOAT,
					.RTTexture = Gfx::GetShaderRT(m_SkyboxShader, 0),
					.LoadRenderPassOp = Gfx::RenderPassOp::Load
				}
			},
			.DepthFormat = {
				.RTFormat = Gfx::Format::D32_SFLOAT,
				.RTTexture = Gfx::GetShaderDepthTarget(m_SkyboxShader),
				.LoadRenderPassOp = Gfx::RenderPassOp::Load
			},
			.Type = Gfx::ShaderType::Graphics
		});

		// Composite shader
		m_CompositeShader = Gfx::CreateShader({
			.ProgramName = "scenecomposite",
			.Type = Gfx::ShaderType::Graphics
		});

		m_SSAO			= std::make_unique<SSAO>();
		m_PathTracing	= std::make_unique<PathTracing>();
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

		for (uint8 i = 0; i < NUM_BACK_BUFFERS; ++i)
			DestroyBuffer(m_SceneInfoBuffers[i]);

		DestroyBuffer(m_ScenesMaterials);
		DestroyBuffer(m_SceneInstances);

		DestroyScene(m_SkyboxCube);
		for (Scene* scene : m_Scenes)
			DestroyScene(scene);
	}

	void SceneRenderer::Render(float dt)
	{
		UpdateSceneInfo();

		BeginEvent("Loading Environment Map");
		if (bNeedsEnvMapChange)
		{
			LoadEnvironmentMap(EnvironmentMaps[Tweaks.SelectedEnvMapIdx].string().c_str());
			bNeedsEnvMapChange = false;
		}
		EndEvent();

		if (Tweaks.CurrentRenderPath == (int)RenderPath::Deferred)
		{
			BeginEvent("Geometry Pass");
			BindShader(m_DeferredShadingShader);
			SetParameter(m_DeferredShadingShader, "LinearWrap", GetDefaultLinearWrapSampler());
			BindSceneInfo(m_DeferredShadingShader);
			for (Scene* scene : m_Scenes)
			{
				scene->IterateMeshes([&](const Mesh& mesh)
				{
					SetParameter(m_DeferredShadingShader, "model", mesh.Transform);
					SetParameter(m_DeferredShadingShader, "instanceID", mesh.InstanceID);

					BindVertexBuffer(mesh.VertexBuffer);
					BindIndexBuffer(mesh.IndexBuffer);
					DrawIndexed((uint32)mesh.IndexCount);
				});
			}
			EndEvent();

			if (Tweaks.bEnableSSAO)
			{
				m_SSAO->Render(this, GetShaderRT(m_DeferredShadingShader, 0), GetShaderDepthTarget(m_DeferredShadingShader));
			}

			BeginEvent("Render Skybox");
			BindShader(m_SkyboxShader);
			BindSceneInfo(m_SkyboxShader);
			SetParameter(m_SkyboxShader, "g_EnvironmentCube", m_EnvironmentCubemap);
			SetParameter(m_SkyboxShader, "LinearWrap", GetDefaultLinearWrapSampler());
			m_SkyboxCube->IterateMeshes([&](const Mesh& mesh)
			{
				BindVertexBuffer(mesh.VertexBuffer);
				BindIndexBuffer(mesh.IndexBuffer);
				DrawIndexed((uint32)mesh.IndexCount);
			});
			EndEvent();

			BeginEvent("PBR Lighting");
			BindShader(m_PBRShader);
			BindSceneInfo(m_PBRShader);
			SetParameter(m_PBRShader, "sceneToRender", Tweaks.CurrentSceneView);
			SetParameter(m_PBRShader, "bEnableSSAO", Tweaks.bEnableSSAO ? 1 : 0);
			// PBR scene info
			SetParameter(m_PBRShader, "lightPos", Light.Position);
			SetParameter(m_PBRShader, "lightColor", Light.Color);
			SetParameter(m_PBRShader, "LinearClamp", GetDefaultLinearClampSampler());
			// Bind deferred shading render targets
			SetParameter(m_PBRShader, "g_WorldPosition", m_DeferredShadingShader, 1);
			SetParameter(m_PBRShader, "g_Albedo", m_DeferredShadingShader, 2);
			SetParameter(m_PBRShader, "g_Normal", m_DeferredShadingShader, 3);
			SetParameter(m_PBRShader, "g_RoughnessMetallicAO", m_DeferredShadingShader, 4);
			SetParameter(m_PBRShader, "g_Emissive", m_DeferredShadingShader, 5);
			SetParameter(m_PBRShader, "g_SSAO", m_SSAO->GetBlurredTexture());
			// Bind irradiance map
			SetParameter(m_PBRShader, "g_IrradianceMap", m_IrradianceMap);
			SetParameter(m_PBRShader, "g_PrefilterMap", m_PrefilterMap);
			SetParameter(m_PBRShader, "g_LUT", m_BRDFLUTMap);
			Draw(6);
			EndEvent();

			m_SceneTexture = GetShaderRT(m_PBRShader, 0);
		}
		else if (Tweaks.CurrentRenderPath == (int)RenderPath::PathTracing)
		{
			m_PathTracing->RebuildAccelerationStructure(m_Scenes);
			m_PathTracing->Render(this, Camera);
			m_SceneTexture = m_PathTracing->GetFinalTexture();
		}

		// render scene composite
		BeginEvent("Scene Composite");
		BindShader(m_CompositeShader);
		SetParameter(m_CompositeShader, "g_TonemapMode", Tweaks.CurrentTonemap);
		SetParameter(m_CompositeShader, "g_sceneTexture", m_SceneTexture);
		SetParameter(m_CompositeShader, "LinearWrap", GetDefaultLinearWrapSampler());
		Draw(6);
		EndEvent();

		Gfx::Present(Tweaks.bEnableVSync);
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

		auto uploadArrayToGPU = [](Handle<Buffer>& buffer, const auto& arr, const char* debugName)
		{
			if (buffer.IsValid())
				DestroyBuffer(buffer);

			buffer = CreateBuffer({
				.DebugName = debugName,
				.NumElements = (uint32)arr.size(),
				.ByteStride = sizeof(arr[0]),
				.ByteSize = arr.size() * sizeof(arr[0]),
				.Usage = BufferUsage::Structured,
				.InitialData = arr.data(),
				.bCreateView = true
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
				instances.emplace_back((uint32)materials.size() + mesh.LocalMaterialIndex);
				instanceID++;
			});

			appendArrays(materials, scene->Materials);
		}

		uploadArrayToGPU(m_ScenesMaterials, materials, "ScenesMaterials");
		uploadArrayToGPU(m_SceneInstances,  instances, "SceneInstances");

		m_SceneInfo.MaterialsBufferIndex = GetBuffer(m_ScenesMaterials)->BasicHandle.Index;
		m_SceneInfo.InstancesBufferIndex = GetBuffer(m_SceneInstances)->BasicHandle.Index;
	}

	void SceneRenderer::UpdateSceneInfo()
	{
		m_SceneInfo.View			= Camera.View;
		m_SceneInfo.Projection		= Camera.Proj;
		m_SceneInfo.ViewProjection	= Camera.ViewProj;
		m_SceneInfo.CameraPos		= Camera.Eye;

		Handle<Buffer> currentBuffer = m_SceneInfoBuffers[Device::Ptr->GetCurrentFrameIndex()];
		memcpy(GetMappedData(currentBuffer), &m_SceneInfo, sizeof(SceneInfo));
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

	void SceneRenderer::BindSceneInfo(Handle<Shader> shaderToBind)
	{
		Handle<Buffer> currentBuffer = m_SceneInfoBuffers[Device::Ptr->GetCurrentFrameIndex()];
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

		uint2 equirectangularTextureSize = { 1024, 1024 };
		m_EnvironmentCubemap = CreateTexture({
			.Width = equirectangularTextureSize.x,
			.Height = equirectangularTextureSize.y,
			.DebugName = "Environment Cubemap",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Format::RGBA16_SFLOAT,
			.Type = TextureType::TextureCube,
		});

		// transform an equirectangular map into a cubemap
		{
			BeginEvent("EquirectToCubemap");
			Handle<Texture> equirectangularTexture = CreateTextureFromFile(path, "Equirectangular Texture");
			Handle<Shader> equirectToCubemap = CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "EquirectToCubemap",
				.Type = ShaderType::Compute
			});

			BindShader(equirectToCubemap);
			SetParameter(equirectToCubemap, "g_EnvironmentEquirectangular", equirectangularTexture);
			SetParameter(equirectToCubemap, "g_OutEnvironmentCubemap", m_EnvironmentCubemap);
			SetParameter(equirectToCubemap, "LinearWrap", GetDefaultLinearWrapSampler());
			Dispatch(equirectangularTextureSize.x / 32, equirectangularTextureSize.y / 32, 6);

			DestroyShader(equirectToCubemap);
			DestroyTexture(equirectangularTexture);
			EndEvent();
		}

		// irradiance map
		{
			BeginEvent("DrawIrradianceMap");
			uint2 irradianceSize = { 1024, 1024 };
			m_IrradianceMap = CreateTexture({
				.Width = irradianceSize.x,
				.Height = irradianceSize.y,
				.DebugName = "Irradiance Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Format::RGBA16_SFLOAT,
				.Type = TextureType::TextureCube,
			});

			Handle<Shader> irradianceShader = CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "DrawIrradianceMap",
				.Type = ShaderType::Compute
			});

			BindShader(irradianceShader);
			SetParameter(irradianceShader, "g_EnvironmentCubemap", m_EnvironmentCubemap);
			SetParameter(irradianceShader, "g_IrradianceMap", m_IrradianceMap);
			SetParameter(irradianceShader, "LinearWrap", GetDefaultLinearWrapSampler());
			Dispatch(irradianceSize.x / 32, irradianceSize.y / 32, 6);

			DestroyShader(irradianceShader);
			EndEvent();
		}

		// Pre filter env map
		{
			BeginEvent("PreFilterEnvMap");
			uint2 prefilterSize = { 1024, 1024 };
			uint16 prefilterMipLevels = 5;

			m_PrefilterMap = CreateTexture({
				.Width = prefilterSize.x,
				.Height = prefilterSize.y,
				.MipLevels = prefilterMipLevels,
				.DebugName = "PreFilter Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Format::RGBA16_SFLOAT,
				.Type = TextureType::TextureCube,
			});

			Handle<Shader> prefilterShader = CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "PreFilterEnvMap",
				.Type = ShaderType::Compute
			});

			BindShader(prefilterShader);
			SetParameter(prefilterShader, "g_EnvironmentCubemap", m_EnvironmentCubemap);
			SetParameter(prefilterShader, "LinearWrap", GetDefaultLinearWrapSampler());

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 32);

				SetParameter(prefilterShader, "g_PreFilteredMap", m_PrefilterMap, level);
				SetParameter(prefilterShader, "roughness", level * deltaRoughness);
				Dispatch(numGroups, numGroups, 6);
			}

			DestroyShader(prefilterShader);
			EndEvent();
		}

		// Compute BRDF LUT map
		{
			BeginEvent("ComputeBRDFLUT");
			uint2 brdfLUTMapSize = { 256, 256 };
			m_BRDFLUTMap = CreateTexture({
				.Width = brdfLUTMapSize.x,
				.Height = brdfLUTMapSize.y,
				.DebugName = "BRDF LUT Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Format::RG16_SFLOAT,
				.Type = TextureType::Texture2D,
			});

			Handle<Shader> brdfLUTShader = CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "ComputeBRDFLUT",
				.Type = ShaderType::Compute
			});
			BindShader(brdfLUTShader);
			SetParameter(brdfLUTShader, "LUT", m_BRDFLUTMap);
			Dispatch(brdfLUTMapSize.x / 32, brdfLUTMapSize.y / 32, 6);

			DestroyShader(brdfLUTShader);
			EndEvent();
		}
	}
}
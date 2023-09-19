#include "stdafx.h"
#include "scenerenderer.h"

#include <filesystem>

#include "profiler.h"
#include "scene.h"
#include "uirenderer.h"
#include "core/jobsystem.h"
#include "rhi/device.h"
#include "rhi/rootsignature.h"
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
		m_DeferredShadingRS = new RHI::RootSignature("Deferred Shading RS");
		m_DeferredShadingRS->AddRootConstants(0, 1);
		m_DeferredShadingRS->AddRootCBV(100);
		m_DeferredShadingRS->Create();

		m_DeferredShadingShader = RHI::CreateShader({
			.ProgramName = "deferredshading",
			.RootSignature = m_DeferredShadingRS,
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
		m_SkyboxRS = new RHI::RootSignature("Skybox RS");
		m_SkyboxRS->AddRootCBV(100);
		m_SkyboxRS->AddRootConstants(0, 1);
		m_SkyboxRS->Create(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_SkyboxCube = Scene::Load("assets/models/skybox.glb");
		m_SkyboxShader = RHI::CreateShader({
			.ProgramName = "sky",
			.InputLayout = {
				{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT }
			},
			.RootSignature = m_SkyboxRS,
			.RTFormats = {
				{ RHI::Format::RGBA16_SFLOAT, "PBR RT" }
			},
			.DepthFormat = { RHI::Format::D32_SFLOAT, "PBR Depth" },
			.Type = RHI::ShaderType::Graphics
		});

		// PBR
		m_LightingRS = new RHI::RootSignature("Lighting RS");
		m_LightingRS->AddRootConstants(0, 7);
		m_LightingRS->AddRootCBV(100);
		m_LightingRS->AddRootCBV(101);
		m_LightingRS->AddRootConstants(1, 10);
		m_LightingRS->Create();

		m_PBRShader = RHI::CreateShader({
			.ProgramName = "pbrlighting",
			.RootSignature = m_LightingRS,
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
		m_CompositeRS = new RHI::RootSignature("Composite RS");
		m_CompositeRS->AddRootConstants(0, 2);
		m_CompositeRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
		m_CompositeRS->Create();

		m_CompositeShader = RHI::CreateShader({
			.ProgramName = "scenecomposite",
			.RootSignature = m_CompositeRS,
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

		DestroyBuffer(m_ScenesMaterials);
		DestroyBuffer(m_SceneInstances);

		DestroyScene(m_SkyboxCube);
		for (Scene* scene : m_Scenes)
			DestroyScene(scene);

		delete m_DeferredShadingRS;
		delete m_SkyboxRS;
		delete m_LightingRS;
		delete m_CompositeRS;
	}

	void SceneRenderer::RenderGeometryPass()
	{
		RHI::BeginProfileEvent("Geometry Pass");
		RHI::BindShader(m_DeferredShadingShader);

		RHI::BindTempConstantBuffer(1, SceneInfo);
		for (Scene* scene : m_Scenes)
		{
			scene->IterateMeshes([&](const Mesh& mesh)
			{
				RHI::BindConstants(0, 0, mesh.InstanceID);

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

		RHI::BindTempConstantBuffer(0, SceneInfo);

		RHI::BindConstants(1, 0, RHI::GetTexture(m_EnvironmentCubemap)->SRV());

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
		
		RHI::BindTempConstantBuffer(1, SceneInfo);
		RHI::BindTempConstantBuffer(2, m_ShadowMapping->GetShadowData());

		RHI::BindConstants(0, 0, Tweaks.CurrentAOTechnique);

		// PBR scene info
		RHI::BindConstants(0, 1, Light.Position);
		RHI::BindConstants(0, 4, Light.Color);


		// Bind deferred shading render targets
		RHI::BindConstants(3, 0, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 0))->SRV());
		RHI::BindConstants(3, 1, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 1))->SRV());
		RHI::BindConstants(3, 2, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 2))->SRV());
		RHI::BindConstants(3, 3, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 3))->SRV());
		RHI::BindConstants(3, 4, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 4))->SRV());
		RHI::BindConstants(3, 5, RHI::GetTexture(GetShaderRT(m_DeferredShadingShader, 5))->SRV());
		if (Tweaks.CurrentAOTechnique == (int)AmbientOcclusion::RTAO)
			RHI::BindConstants(3, 6, RHI::GetTexture(m_RTAO->GetFinalTexture())->SRV());
		else
			RHI::BindConstants(3, 6, RHI::GetTexture(m_SSAO->GetBlurredTexture())->SRV());
		RHI::BindConstants(3, 7, RHI::GetTexture(m_IrradianceMap)->SRV());
		RHI::BindConstants(3, 8, RHI::GetTexture(m_PrefilterMap)->SRV());
		RHI::BindConstants(3, 9, RHI::GetTexture(m_BRDFLUTMap)->SRV());

		RHI::Draw(6);
		RHI::EndProfileEvent("Lighting");
	}

	void SceneRenderer::RenderSceneCompositePass()
	{
		RHI::BeginProfileEvent("Scene Composite");
		RHI::BindShader(m_CompositeShader);
		RHI::BindConstants(0, 0, Tweaks.CurrentTonemap);

		RHI::BindConstants(0, 1, RHI::GetTexture(m_SceneTexture)->SRV());

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
				instance.BufferIndex	 = scene->GetGeometryBuffer()->CBVHandle.Index;
				instanceID++;
			});

			appendArrays(materials, scene->Materials);
		}

		uploadArrayToGPU(m_ScenesMaterials, materials, "ScenesMaterials");
		uploadArrayToGPU(m_SceneInstances,  instances, "SceneInstances");

		SceneInfo.MaterialsBufferIndex = GetBuffer(m_ScenesMaterials)->CBVHandle.Index;
		SceneInfo.InstancesBufferIndex = GetBuffer(m_SceneInstances)->CBVHandle.Index;
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
		SceneInfo.SkyIndex				= GetTexture(m_EnvironmentCubemap)->SRV();
		SceneInfo.SceneViewToRender		= Tweaks.CurrentSceneView;
		SceneInfo.FrameIndex++;
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
			std::unique_ptr<RHI::RootSignature> m_TempRS = std::make_unique<RHI::RootSignature>("EquirectToCubemap Temp RS");
			m_TempRS->AddRootConstants(0, 1);
			m_TempRS->AddRootConstants(1, 1);
			m_TempRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
			m_TempRS->Create();

			RHI::BeginEvent("EquirectToCubemap");
			RHI::Handle<RHI::Texture> equirectangularTexture = RHI::CreateTextureFromFile(path, "Equirectangular Texture");
			RHI::Handle<RHI::Shader> equirectToCubemap = RHI::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "EquirectToCubemap",
				.RootSignature = m_TempRS.get(),
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(equirectToCubemap);

			RHI::BindConstants(1, 0, RHI::GetTexture(equirectangularTexture)->SRV());

			RHI::DescriptorHandle uavTexture[] =
			{
				RHI::GetTexture(m_EnvironmentCubemap)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(2, uavTexture, 1);
			RHI::Dispatch(equirectangularTextureSize.x / 8, equirectangularTextureSize.y / 8, 6);

			RHI::DestroyShader(equirectToCubemap);
			RHI::DestroyTexture(equirectangularTexture);
			RHI::EndEvent();
		}

		// irradiance map
		{
			std::unique_ptr<RHI::RootSignature> m_TempRS = std::make_unique<RHI::RootSignature>("DrawIrradianceMap Temp RS");
			m_TempRS->AddRootConstants(0, 1);
			m_TempRS->AddRootConstants(1, 1);
			m_TempRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
			m_TempRS->Create();

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
				.RootSignature = m_TempRS.get(),
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(irradianceShader);
			RHI::BindConstants(0, 0, RHI::GetTexture(m_EnvironmentCubemap)->SRV());

			RHI::DescriptorHandle uavTexture[] =
			{
				RHI::GetTexture(m_IrradianceMap)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(2, uavTexture, 1);
			RHI::Dispatch(irradianceSize.x / 8, irradianceSize.y / 8, 6);

			RHI::DestroyShader(irradianceShader);
			RHI::EndEvent();
		}

		// Pre filter env map
		{
			std::unique_ptr<RHI::RootSignature> m_TempRS = std::make_unique<RHI::RootSignature>("PreFilterEnvMap Temp RS");
			m_TempRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
			m_TempRS->AddRootConstants(0, 2);
			m_TempRS->AddRootConstants(1, 1);
			m_TempRS->AddRootConstants(2, 1);
			m_TempRS->Create();

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
				.RootSignature = m_TempRS.get(),
				.Type = RHI::ShaderType::Compute
			});

			RHI::BindShader(prefilterShader);
			RHI::BindConstants(1, 0, RHI::GetTexture(m_EnvironmentCubemap)->SRV());

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 8);

				RHI::DescriptorHandle uavTexture[] =
				{
					RHI::GetTexture(m_PrefilterMap)->UAVHandle[level]
				};
				RHI::BindTempDescriptorTable(0, uavTexture, 1);
				RHI::BindConstants(2, 0, level * deltaRoughness);
				RHI::Dispatch(numGroups, numGroups, 6);
			}

			RHI::DestroyShader(prefilterShader);
			RHI::EndEvent();
		}

		// Compute BRDF LUT map
		{
			std::unique_ptr<RHI::RootSignature> m_TempRS = std::make_unique<RHI::RootSignature>("ComputeBRDFLUT Temp RS");
			m_TempRS->AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
			m_TempRS->Create();

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
				.RootSignature = m_TempRS.get(),
				.Type = RHI::ShaderType::Compute
			});
			RHI::BindShader(brdfLUTShader);
			RHI::DescriptorHandle uavTexture[] =
			{
				RHI::GetTexture(m_BRDFLUTMap)->UAVHandle[0]
			};
			RHI::BindTempDescriptorTable(0, uavTexture, 1);
			RHI::Dispatch(brdfLUTMapSize.x / 8, brdfLUTMapSize.y / 8, 6);

			RHI::DestroyShader(brdfLUTShader);
			RHI::EndEvent();
		}
	}
}

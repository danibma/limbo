#include "stdafx.h"

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/scene.h"
#include "gfx/fpscamera.h"

#include "core/timer.h"
#include "core/window.h"
#include "core/commandline.h"

#include "tests/tests.h"

#include <array>
#include <filesystem>


using namespace limbo;

#define WIDTH  1280
#define HEIGHT 720

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PSTR lpCmdLine, int nCmdShow)
{
	Core::CommandLine::Init(lpCmdLine);
	if (Core::CommandLine::HasArg("--tests"))
		return Tests::ExecuteTests(lpCmdLine);
	else if (Core::CommandLine::HasArg("--ctriangle"))
		return Tests::ExecuteComputeTriangle();
	else if (Core::CommandLine::HasArg("--gtriangle"))
		return Tests::ExecuteGraphicsTriangle();

	Core::Window* window = Core::NewWindow({
		.Title = "limbo",
		.Width = WIDTH,
		.Height = HEIGHT
	});

	Gfx::Init(window, Gfx::GfxDeviceFlag::EnableImgui);

	Gfx::FPSCamera camera = Gfx::CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

	// Deferred shader
	Gfx::Handle<Gfx::Shader> deferredShader = Gfx::CreateShader({
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

	// IBL stuff
	Gfx::Handle<Gfx::Texture> environmentCubemap;
	Gfx::Handle<Gfx::Texture> irradianceMap;
	Gfx::Handle<Gfx::Texture> prefilterMap;
	Gfx::Handle<Gfx::Texture> brdfLUTMap;

	auto loadSkyboxTexture = [&environmentCubemap, &irradianceMap, &prefilterMap, &brdfLUTMap](const char* hdrTexture) -> void
	{
		if (environmentCubemap.IsValid())
			Gfx::DestroyTexture(environmentCubemap);
		if (irradianceMap.IsValid())
			Gfx::DestroyTexture(irradianceMap);
		if (prefilterMap.IsValid())
			Gfx::DestroyTexture(prefilterMap);
		if (brdfLUTMap.IsValid())
			Gfx::DestroyTexture(brdfLUTMap);

		uint2 equirectangularTextureSize = { 1024, 1024 };
		environmentCubemap = Gfx::CreateTexture({
			.Width = equirectangularTextureSize.x,
			.Height = equirectangularTextureSize.y,
			.DebugName = "Environment Cubemap",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Gfx::Format::RGBA16_SFLOAT,
			.Type = Gfx::TextureType::TextureCube,
		});

		// transform an equirectangular map into a cubemap
		{
			Gfx::BeginEvent("EquirectToCubemap");
			Gfx::Handle<Gfx::Texture> equirectangularTexture = Gfx::CreateTextureFromFile(hdrTexture, "Equirectangular Texture");
			Gfx::Handle<Gfx::Shader> equirectToCubemap = Gfx::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "EquirectToCubemap",
				.Type = Gfx::ShaderType::Compute
			});

			Gfx::BindShader(equirectToCubemap);
			Gfx::SetParameter(equirectToCubemap, "g_EnvironmentEquirectangular", equirectangularTexture);
			Gfx::SetParameter(equirectToCubemap, "g_OutEnvironmentCubemap", environmentCubemap);
			Gfx::SetParameter(equirectToCubemap, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
			Gfx::Dispatch(equirectangularTextureSize.x / 32, equirectangularTextureSize.y / 32, 6);

			Gfx::DestroyShader(equirectToCubemap);
			Gfx::DestroyTexture(equirectangularTexture);
			Gfx::EndEvent();
		}

		// irradiance map
		{
			Gfx::BeginEvent("DrawIrradianceMap");
			uint2 irradianceSize = { 1024, 1024 };
			irradianceMap = Gfx::CreateTexture({
				.Width = irradianceSize.x,
				.Height = irradianceSize.y,
				.DebugName = "Irradiance Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Gfx::Format::RGBA16_SFLOAT,
				.Type = Gfx::TextureType::TextureCube,
			});

			Gfx::Handle<Gfx::Shader> irradianceShader = Gfx::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "DrawIrradianceMap",
				.Type = Gfx::ShaderType::Compute
			});

			Gfx::BindShader(irradianceShader);
			Gfx::SetParameter(irradianceShader, "g_EnvironmentCubemap", environmentCubemap);
			Gfx::SetParameter(irradianceShader, "g_IrradianceMap", irradianceMap);
			Gfx::SetParameter(irradianceShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
			Gfx::Dispatch(irradianceSize.x / 32, irradianceSize.y / 32, 6);

			Gfx::DestroyShader(irradianceShader);
			Gfx::EndEvent();
		}

		// Pre filter env map
		{
			Gfx::BeginEvent("PreFilterEnvMap");
			uint2 prefilterSize			= { 1024, 1024 };
			uint16 prefilterMipLevels	= 5;

			prefilterMap = Gfx::CreateTexture({
				.Width = prefilterSize.x,
				.Height = prefilterSize.y,
				.MipLevels = prefilterMipLevels,
				.DebugName = "PreFilter Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Gfx::Format::RGBA16_SFLOAT,
				.Type = Gfx::TextureType::TextureCube,
			});

			Gfx::Handle<Gfx::Shader> prefilterShader = Gfx::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "PreFilterEnvMap",
				.Type = Gfx::ShaderType::Compute
			});

			Gfx::BindShader(prefilterShader);
			Gfx::SetParameter(prefilterShader, "g_EnvironmentCubemap", environmentCubemap);
			Gfx::SetParameter(prefilterShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 32);

				Gfx::SetParameter(prefilterShader, "g_PreFilteredMap", prefilterMap, level);
				Gfx::SetParameter(prefilterShader, "roughness", level * deltaRoughness);
				Gfx::Dispatch(numGroups, numGroups, 6);
			}

			Gfx::DestroyShader(prefilterShader);
			Gfx::EndEvent();
		}

		// Compute BRDF LUT map
		{
			Gfx::BeginEvent("ComputeBRDFLUT");
			uint2 brdfLUTMapSize = { 256, 256 };
			brdfLUTMap = Gfx::CreateTexture({
				.Width = brdfLUTMapSize.x,
				.Height = brdfLUTMapSize.y,
				.DebugName = "BRDF LUT Map",
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.Format = Gfx::Format::RG16_SFLOAT,
				.Type = Gfx::TextureType::Texture2D,
			});

			Gfx::Handle<Gfx::Shader> brdfLUTShader = Gfx::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "ComputeBRDFLUT",
				.Type = Gfx::ShaderType::Compute
				});
			Gfx::BindShader(brdfLUTShader);
			Gfx::SetParameter(brdfLUTShader, "LUT", brdfLUTMap);
			Gfx::Dispatch(brdfLUTMapSize.x / 32, brdfLUTMapSize.y / 32, 6);

			Gfx::DestroyShader(brdfLUTShader);
			Gfx::EndEvent();
		}
	};

	// Environment Maps
	int selectedEnvMap = 4;
	bool bChangeEnvMap = true;
	std::vector<std::filesystem::path> envMaps;
	const char* env_maps_path = "assets/environment";
	for (const auto& entry : std::filesystem::directory_iterator(env_maps_path))
		envMaps.emplace_back(entry.path());

	// Skybox
	Gfx::Handle<Gfx::Shader> skyboxShader;
	Gfx::Scene* skyboxCube = Gfx::Scene::Load("assets/models/skybox.glb");
	{
		skyboxShader = Gfx::CreateShader({
			.ProgramName = "sky",
			.RTFormats = {
				{ Gfx::Format::RGBA16_SFLOAT, "PBR RT" }
			},
			.DepthFormat = { Gfx::Format::D32_SFLOAT, "PBR Depth" },
			.Type = Gfx::ShaderType::Graphics
		});
	}

	// PBR
	Gfx::Handle<Gfx::Shader> pbrShader = Gfx::CreateShader({
		.ProgramName = "pbrlighting",
		.RTFormats = {
			{
				.RTFormat = Gfx::Format::RGBA16_SFLOAT,
				.RTTexture = Gfx::GetShaderRT(skyboxShader, 0),
				.LoadRenderPassOp = Gfx::RenderPassOp::Load
			}
		},
		.DepthFormat = {
			.RTFormat = Gfx::Format::D32_SFLOAT,
			.RTTexture = Gfx::GetShaderDepthTarget(skyboxShader),
			.LoadRenderPassOp = Gfx::RenderPassOp::Load
		},
		.Type = Gfx::ShaderType::Graphics
	});

	// Composite shader
	Gfx::Handle<Gfx::Shader> compositeShader = Gfx::CreateShader({
		.ProgramName = "scenecomposite",
		.Type = Gfx::ShaderType::Graphics
	});

	std::vector<Gfx::Scene*> scenes;
	//scenes.emplace_back(Gfx::Scene::Load("assets/models/Sponza/Sponza.gltf"));

	int tonemapMode = 1;
	const char* tonemapModes[] = {
		"None",
		"Aces Film",
		"Reinhard"
	};

	// Debug views
	std::array<const char*, 8> debug_views = { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive", "Ambient Occlusion" };
	int selected_debug_view = 0;

	float3 lightPosition(0.0f, 1.0f, 0.0f);
	float3 lightColor(1, 0.45f, 0);

	// SSAO
	Gfx::Handle<Gfx::Shader> SSAOShader = Gfx::CreateShader({
		.ProgramName = "ssao",
		.RTFormats = { { Gfx::Format::R32_SFLOAT, "SSAO RT" } },
	});

	Gfx::Handle<Gfx::Texture> blurredSSAOTexture = Gfx::CreateTexture({
		.Width = window->Width,
		.Height = window->Height,
		.DebugName = "Blurred SSAO Texture",
		.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		.Format = Gfx::Format::R32_SFLOAT,
	});
	Gfx::Handle<Gfx::Shader> BlurSSAOShader = Gfx::CreateShader({
		.ProgramName = "ssao",
		.CsEntryPoint = "BlurSSAO",
		.Type = Gfx::ShaderType::Compute
	});

	// Generate the Sample Kernel for SSAO - https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html
	Gfx::Handle<Gfx::Texture> ssaoNoiseTexture;
	Gfx::Handle<Gfx::Buffer> ssaoSampleKernel;
	{
		constexpr uint32 kernelSize = 64;
		float4 ssaoSamplekernel[kernelSize];
		for (int i = 0; i < kernelSize; ++i) 
		{
			ssaoSamplekernel[i].w = 0;

			ssaoSamplekernel[i].x = Random::Float(i * kernelSize + 0, -1.0f, 1.0f);
			ssaoSamplekernel[i].y = Random::Float(i * kernelSize + 1, -1.0f, 1.0f);
			ssaoSamplekernel[i].z = Random::Float(i * kernelSize + 2,  0.0f, 1.0f);
			glm::normalize(ssaoSamplekernel[i]);
			ssaoSamplekernel[i].x *= Random::Float(i * kernelSize + 3, 0.0f, 1.0f);
			ssaoSamplekernel[i].y *= Random::Float(i * kernelSize + 4, 0.0f, 1.0f);
			ssaoSamplekernel[i].z *= Random::Float(i * kernelSize + 5, 0.0f, 1.0f);

			float scale = float(i) / float(kernelSize);
			scale = glm::mix(0.1f, 1.0f, scale * scale);
			ssaoSamplekernel[i] *= scale;
		}

		ssaoSampleKernel = Gfx::CreateBuffer({
			.DebugName = "SSAO Sample Kernel",
			.ByteSize = sizeof(float4) * 64,
			.Usage = Gfx::BufferUsage::Constant,
			.InitialData = ssaoSamplekernel
		});

		// SSAO Noise texture
		constexpr uint32 noiseSize = 256;
		float4 ssaoNoise[noiseSize];
		for (int i = 0; i < noiseSize; ++i)
		{
			ssaoNoise[i].x = Random::Float(i * kernelSize + (noiseSize - i) + 0, -1.0f, 1.0f);
			ssaoNoise[i].y = Random::Float(i * kernelSize + (noiseSize - i) + 1, -1.0f, 1.0f);
			ssaoNoise[i].z = 0.0f;
			ssaoNoise[i].w = 0.0f;
			glm::normalize(ssaoNoise[i]);
		}

		ssaoNoiseTexture = Gfx::CreateTexture({
			.Width = 4,
			.Height = 4,
			.MipLevels = 1,
			.DebugName = "SSAO Noise Texture",
			.Format = Gfx::Format::RGB32_SFLOAT,
			.Type = Gfx::TextureType::Texture2D,
			.InitialData = ssaoNoise,
		});
	}

	bool bEnableSSAO = true;
	float radius = 0.4f;

	Core::Timer deltaTimer;
	while (!window->ShouldClose())
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();


		window->PollEvents();

		Gfx::UpdateCamera(window, camera, deltaTime);

#pragma region Input
		{
			bool ReloadShadersBind = Input::IsKeyDown(window, Input::KeyCode::LeftControl) && Input::IsKeyDown(window, Input::KeyCode::R);
			if (ReloadShadersBind)
				Gfx::ReloadShaders();
		}
#pragma endregion Input

#pragma region UI
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Load"))
				{
					std::vector<wchar_t*> results;
					if (Utils::OpenFileDialog(window, L"Choose scene to load", results, L"", { L"*.gltf; *.glb"}))
					{
						char path[MAX_PATH];
						Utils::StringConvert(results[0], path);
						scenes.emplace_back(Gfx::Scene::Load(path));
					}
				}

				bool bCanClearScene = scenes.size() > 0;
				if (ImGui::MenuItem("Clear Scene", "", false, bCanClearScene))
				{
					for (Gfx::Scene* scene : scenes)
						Gfx::DestroyScene(scene);
					scenes.clear();
				}

				ImGui::SeparatorText("Environment Map");
				{
					std::string envPreviewValue = envMaps[selectedEnvMap].stem().string();
					if (ImGui::BeginCombo("##env_map", envPreviewValue.c_str()))
					{
						for (int i = 0; i < envMaps.size(); i++)
						{
							const bool bIsSelected = (selectedEnvMap == i);
							if (ImGui::Selectable(envMaps[i].stem().string().c_str(), bIsSelected))
							{
								if (selectedEnvMap != i)
									bChangeEnvMap = true;
								selectedEnvMap = i;
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (bIsSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}
				

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Renderer"))
			{
				if (Gfx::CanTakeGPUCapture() && ImGui::MenuItem("Take GPU Capture"))
					Gfx::TakeGPUCapture();

				if (Gfx::CanTakeGPUCapture() && ImGui::MenuItem("Open Last GPU Capture"))
					Gfx::OpenLastGPUCapture();

				if (ImGui::MenuItem("Reload Shaders", "Ctrl-R"))
					Gfx::ReloadShaders();

				ImGui::EndMenu();
			}

			char menuText[256];
			snprintf(menuText, 256, "Device: %s | CPU Time: %.2f ms (%.2f fps)", Gfx::GetGPUInfo().Name, deltaTime, 1000.0f / deltaTime);
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(menuText).x) - 10.0f);
			ImGui::Text(menuText);

			ImGui::EndMainMenuBar();
		}

		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::Begin("Limbo##debugwindow", nullptr);
		{
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Position: %.1f, %.1f, %.1f", camera.Eye.x, camera.Eye.y, camera.Eye.z);
				ImGui::PushItemWidth(150.0f);
				ImGui::DragFloat("Speed", &camera.CameraSpeed, 0.1f, 0.1f);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Set to light pos"))
					camera.Eye = lightPosition;
			}

			if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Light Position", &lightPosition[0], 0.1f);
				ImGui::ColorEdit3("Light Color", &lightColor[0]);
			}

			if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushItemWidth(200.0f);
				ImGui::Combo("Tonemap", &tonemapMode, tonemapModes, 3);

				const char* combo_preview_value = debug_views[selected_debug_view];
				if (ImGui::BeginCombo("Debug Views##debug_view", combo_preview_value))
				{
					for (int i = 0; i < debug_views.size(); i++)
					{
						const bool is_selected = (selected_debug_view == i);
						if (ImGui::Selectable(debug_views[i], is_selected))
							selected_debug_view = i;

						// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				ImGui::Checkbox("Enable SSAO", &bEnableSSAO);
				ImGui::DragFloat("SSAO Radius", &radius, 0.1f);
			}
			
			ImGui::End();
		}
#pragma endregion UI

		Gfx::BeginEvent("Loading Environment Map");
		if (bChangeEnvMap)
		{
			loadSkyboxTexture(envMaps[selectedEnvMap].string().c_str());
			bChangeEnvMap = false;
		}
		Gfx::EndEvent();

		Gfx::BeginEvent("Geometry Pass");
		Gfx::BindShader(deferredShader);
		Gfx::SetParameter(deferredShader, "view", camera.View);
		Gfx::SetParameter(deferredShader, "viewProj", camera.ViewProj);
		Gfx::SetParameter(deferredShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
		Gfx::SetParameter(deferredShader, "camPos", camera.Eye);
		for (Gfx::Scene* scene : scenes)
		{
			scene->DrawMesh([&](const Gfx::Mesh& mesh)
			{
				const Gfx::MeshMaterial& material = scene->GetMaterial(mesh.MaterialID);

				Gfx::SetParameter(deferredShader, "g_albedoTexture", material.Albedo);
				Gfx::SetParameter(deferredShader, "g_normalTexture", material.Normal);
				Gfx::SetParameter(deferredShader, "g_roughnessMetalTexture", material.RoughnessMetal);
				Gfx::SetParameter(deferredShader, "g_emissiveTexture", material.Emissive);
				Gfx::SetParameter(deferredShader, "g_AOTexture", material.AmbientOcclusion);
				Gfx::SetParameter(deferredShader, "model", mesh.Transform);
				Gfx::SetParameter(deferredShader, "g_MaterialFactors", material.Factors);

				Gfx::BindVertexBuffer(mesh.VertexBuffer);
				Gfx::BindIndexBuffer(mesh.IndexBuffer);
				Gfx::DrawIndexed((uint32)mesh.IndexCount);
			});
		}
		Gfx::EndEvent();

		if (bEnableSSAO)
		{
			Gfx::BeginEvent("SSAO");
			Gfx::BindShader(SSAOShader);
			Gfx::SetParameter(SSAOShader, "g_Positions", deferredShader, 0);
			Gfx::SetParameter(SSAOShader, "g_Normals", deferredShader, 3);
			Gfx::SetParameter(SSAOShader, "g_SSAONoise", ssaoNoiseTexture);
			Gfx::SetParameter(SSAOShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
			Gfx::SetParameter(SSAOShader, "noiseScale", float2(Gfx::GetBackbufferWidth() / 4.0f, Gfx::GetBackbufferHeight() / 4.0f));
			Gfx::SetParameter(SSAOShader, "radius", radius);
			Gfx::SetParameter(SSAOShader, "projection", camera.Proj);
			Gfx::SetParameter(SSAOShader, "_SampleKernel", ssaoSampleKernel);
			Gfx::Draw(6);
			Gfx::EndEvent();

			Gfx::BeginEvent("SSAO Blur Texture");
			Gfx::BindShader(BlurSSAOShader);
			Gfx::SetParameter(BlurSSAOShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
			Gfx::SetParameter(BlurSSAOShader, "g_SSAOTexture", SSAOShader, 0);
			Gfx::SetParameter(BlurSSAOShader, "g_BlurredSSAOTexture", blurredSSAOTexture);
			Gfx::Dispatch(window->Width / 8, window->Height / 8, 1);
			Gfx::EndEvent();
		}

		Gfx::BeginEvent("Render Skybox");
		Gfx::BindShader(skyboxShader);
		Gfx::SetParameter(skyboxShader, "g_EnvironmentCube", environmentCubemap);
		Gfx::SetParameter(skyboxShader, "view", camera.View);
		Gfx::SetParameter(skyboxShader, "proj", camera.Proj);
		Gfx::SetParameter(skyboxShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
		skyboxCube->DrawMesh([&](const Gfx::Mesh& mesh)
		{
			Gfx::BindVertexBuffer(mesh.VertexBuffer);
			Gfx::BindIndexBuffer(mesh.IndexBuffer);
			Gfx::DrawIndexed((uint32)mesh.IndexCount);
		});
		Gfx::EndEvent();

		Gfx::BeginEvent("PBR Lighting");
		Gfx::BindShader(pbrShader);
		Gfx::SetParameter(pbrShader, "sceneToRender", selected_debug_view);
		Gfx::SetParameter(pbrShader, "bEnableSSAO", bEnableSSAO ? 1 : 0);
		// PBR scene info
		Gfx::SetParameter(pbrShader, "camPos", camera.Eye);
		Gfx::SetParameter(pbrShader, "lightPos", lightPosition);
		Gfx::SetParameter(pbrShader, "lightColor", lightColor);
		Gfx::SetParameter(pbrShader, "LinearClamp", Gfx::GetDefaultLinearClampSampler());
		// Bind deferred shading render targets
		Gfx::SetParameter(pbrShader, "g_WorldPosition",			deferredShader, 1);
		Gfx::SetParameter(pbrShader, "g_Albedo",				deferredShader, 2);
		Gfx::SetParameter(pbrShader, "g_Normal",				deferredShader, 3);
		Gfx::SetParameter(pbrShader, "g_RoughnessMetallicAO",	deferredShader, 4);
		Gfx::SetParameter(pbrShader, "g_Emissive",				deferredShader, 5);
		Gfx::SetParameter(pbrShader, "g_SSAO",					blurredSSAOTexture);
		// Bind irradiance map
		Gfx::SetParameter(pbrShader, "g_IrradianceMap", irradianceMap);
		Gfx::SetParameter(pbrShader, "g_PrefilterMap", prefilterMap);
		Gfx::SetParameter(pbrShader, "g_LUT", brdfLUTMap);
		Gfx::Draw(6);
		Gfx::EndEvent();

		// render scene composite
		Gfx::BeginEvent("Scene Composite");
		Gfx::BindShader(compositeShader);
		Gfx::SetParameter(compositeShader, "g_TonemapMode", tonemapMode);
		Gfx::SetParameter(compositeShader, "g_sceneTexture", pbrShader, 0);
		Gfx::SetParameter(compositeShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
		Gfx::Draw(6);
		Gfx::EndEvent();

		Gfx::Present();
	}

	Gfx::DestroyTexture(environmentCubemap);
	Gfx::DestroyTexture(irradianceMap);
	Gfx::DestroyTexture(prefilterMap);
	Gfx::DestroyTexture(brdfLUTMap);

	Gfx::DestroyTexture(ssaoNoiseTexture);
	Gfx::DestroyTexture(blurredSSAOTexture);
	Gfx::DestroyBuffer(ssaoSampleKernel);

	Gfx::DestroyShader(pbrShader);
	Gfx::DestroyShader(skyboxShader);
	Gfx::DestroyShader(deferredShader);
	Gfx::DestroyShader(compositeShader);
	Gfx::DestroyShader(SSAOShader);
	Gfx::DestroyShader(BlurSSAOShader);

	Gfx::DestroyScene(skyboxCube);
	for (Gfx::Scene* scene : scenes)
		Gfx::DestroyScene(scene);

	Gfx::Shutdown();
	Core::DestroyWindow(window);

	return 0;
}
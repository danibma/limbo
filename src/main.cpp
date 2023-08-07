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
			{ Gfx::Format::RGBA16_SFLOAT, "WorldPosition"},
			{ Gfx::Format::RGBA16_SFLOAT, "Albedo"},
			{ Gfx::Format::RGBA16_SFLOAT, "Normal" },
			{ Gfx::Format::RGBA16_SFLOAT, "Roughness" },
			{ Gfx::Format::RGBA16_SFLOAT, "Metallic" },
			{ Gfx::Format::RGBA16_SFLOAT, "Emissive" }
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
	int selectedEnvMap = 0;
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
				{ Gfx::Format::RGBA8_UNORM, "PBR RT" }
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
				.RTFormat = Gfx::Format::RGBA8_UNORM,
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
	std::array<const char*, 7> debug_views = { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive" };
	int selected_debug_view = 0;

	float3 lightPosition(0.0f, 1.0f, 0.0f);
	float3 lightColor(0, 1, 0);

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
				if (ImGui::BeginMenu("Camera"))
				{
					ImGui::PushItemWidth(150.0f);
					ImGui::DragFloat("Speed", &camera.CameraSpeed, 0.1f, 0.1f);
					ImGui::PopItemWidth();
					ImGui::EndMenu();
				}
				ImGui::Separator();
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

				if (Gfx::CanTakeGPUCapture() && ImGui::MenuItem("Take GPU Capture"))
					Gfx::TakeGPUCapture();

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

		ImGui::SetNextWindowBgAlpha(0.2f);
		ImGui::SetNextWindowPos(ImVec2(10.0f, 30.0f));
		ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
		if (ImGui::Begin("##debugwindow", nullptr, ImGuiWindowFlags_NoDecoration))
		{
			ImGui::SeparatorText("Camera");
			ImGui::Text("Position: %.1f, %.1f, %.1f", camera.Eye.x, camera.Eye.y, camera.Eye.z);
			ImGui::SameLine();
			if (ImGui::Button("Set to light pos"))
				camera.Eye = lightPosition;
			ImGui::SeparatorText("Light");
			ImGui::DragFloat3("Light Position", &lightPosition[0], 0.1f);
			ImGui::ColorEdit3("Light Color", &lightColor[0]);
			
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
		Gfx::SetParameter(deferredShader, "viewProj", camera.ViewProj);
		Gfx::SetParameter(deferredShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
		for (Gfx::Scene* scene : scenes)
		{
			scene->DrawMesh([&](const Gfx::Mesh& mesh)
			{
				const Gfx::MeshMaterial& material = scene->GetMaterial(mesh.MaterialID);

				Gfx::SetParameter(deferredShader, "g_albedoTexture", material.Albedo);
				Gfx::SetParameter(deferredShader, "g_roughnessMetalTexture", material.RoughnessMetal);
				Gfx::SetParameter(deferredShader, "g_emissiveTexture", material.Emissive);
				Gfx::SetParameter(deferredShader, "model", mesh.Transform);
				Gfx::SetParameter(deferredShader, "g_MaterialFactors", material.Factors);

				Gfx::BindVertexBuffer(mesh.VertexBuffer);
				Gfx::BindIndexBuffer(mesh.IndexBuffer);
				Gfx::DrawIndexed((uint32)mesh.IndexCount);
			});
		}
		Gfx::EndEvent();

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
		// PBR scene info
		Gfx::SetParameter(pbrShader, "camPos", camera.Eye);
		Gfx::SetParameter(pbrShader, "lightPos", lightPosition);
		Gfx::SetParameter(pbrShader, "lightColor", lightColor);
		Gfx::SetParameter(pbrShader, "LinearClamp", Gfx::GetDefaultLinearClampSampler());
		// Bind deferred shading render targets
		Gfx::SetParameter(pbrShader, "g_WorldPosition", deferredShader, 0);
		Gfx::SetParameter(pbrShader, "g_Albedo",		deferredShader, 1);
		Gfx::SetParameter(pbrShader, "g_Normal",		deferredShader, 2);
		Gfx::SetParameter(pbrShader, "g_Roughness",		deferredShader, 3);
		Gfx::SetParameter(pbrShader, "g_Metallic",		deferredShader, 4);
		Gfx::SetParameter(pbrShader, "g_Emissive",		deferredShader, 5);
		// Bind irradiance map
		Gfx::SetParameter(pbrShader, "g_IrradianceMap", irradianceMap);
		Gfx::SetParameter(pbrShader, "g_PrefilterMap", prefilterMap);
		Gfx::SetParameter(pbrShader, "g_LUT", brdfLUTMap);
		Gfx::Draw(6);
		Gfx::EndEvent();

		// render scene composite
		Gfx::Handle<Gfx::Texture> sceneTexture;
		if (selected_debug_view == 1)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 1);
		else if (selected_debug_view == 2)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 2);
		else if (selected_debug_view == 3)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 0);
		else if (selected_debug_view == 4)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 4);
		else if (selected_debug_view == 5)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 3);
		else if (selected_debug_view == 6)
			sceneTexture = Gfx::GetShaderRT(deferredShader, 5);
		else
			sceneTexture = Gfx::GetShaderRT(pbrShader, 0);
		Gfx::BeginEvent("Scene Composite");
		Gfx::BindShader(compositeShader);
		Gfx::SetParameter(compositeShader, "g_TonemapMode", tonemapMode);
		Gfx::SetParameter(compositeShader, "g_sceneTexture", sceneTexture);
		Gfx::SetParameter(compositeShader, "LinearWrap", Gfx::GetDefaultLinearWrapSampler());
		Gfx::Draw(6);
		Gfx::EndEvent();

		Gfx::Present();
	}

	Gfx::DestroyTexture(environmentCubemap);
	Gfx::DestroyTexture(irradianceMap);
	Gfx::DestroyTexture(prefilterMap);
	Gfx::DestroyTexture(brdfLUTMap);

	Gfx::DestroyShader(pbrShader);
	Gfx::DestroyShader(skyboxShader);
	Gfx::DestroyShader(deferredShader);
	Gfx::DestroyShader(compositeShader);

	Gfx::DestroyScene(skyboxCube);
	for (Gfx::Scene* scene : scenes)
		Gfx::DestroyScene(scene);

	Gfx::Shutdown();
	Core::DestroyWindow(window);

	return 0;
}
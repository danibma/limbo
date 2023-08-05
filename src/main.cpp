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

	Gfx::Handle<Gfx::Sampler> linearWrapSampler = Gfx::CreateSampler({
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
		.MinLOD = 0.0f,
		.MaxLOD = 1.0f
	});

	Gfx::Handle<Gfx::Sampler> linearClampSampler = Gfx::CreateSampler({
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
		.MinLOD = 0.0f,
		.MaxLOD = 1.0f
	});

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
	{
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
			Gfx::Handle<Gfx::Texture> equirectangularTexture = Gfx::CreateTextureFromFile("assets/environment/a_rotes_rathaus_4k.hdr", "Equirectangular Texture");
			Gfx::Handle<Gfx::Shader> equirectToCubemap = Gfx::CreateShader({
				.ProgramName = "ibl",
				.CsEntryPoint = "EquirectToCubemap",
				.Type = Gfx::ShaderType::Compute
			});

			Gfx::BindShader(equirectToCubemap);
			Gfx::SetParameter(equirectToCubemap, "g_EnvironmentEquirectangular", equirectangularTexture);
			Gfx::SetParameter(equirectToCubemap, "g_OutEnvironmentCubemap", environmentCubemap);
			Gfx::SetParameter(equirectToCubemap, "LinearWrap", linearWrapSampler);
			Gfx::Dispatch(equirectangularTextureSize.x / 32, equirectangularTextureSize.y / 32, 6);

			Gfx::DestroyShader(equirectToCubemap);
			Gfx::DestroyTexture(equirectangularTexture);
		}

		// irradiance map
		{
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
			Gfx::SetParameter(irradianceShader, "LinearWrap", linearWrapSampler);
			Gfx::Dispatch(irradianceSize.x / 32, irradianceSize.y / 32, 6);

			Gfx::DestroyShader(irradianceShader);
		}

		// Pre filter env map
		{
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
			Gfx::SetParameter(prefilterShader, "LinearWrap", linearWrapSampler);

			const float deltaRoughness = 1.0f / glm::max(float(prefilterMipLevels - 1), 1.0f);
			for (uint32_t level = 0, size = prefilterSize.x; level < prefilterMipLevels; ++level, size /= 2)
			{
				const uint32_t numGroups = glm::max<uint32_t>(1, size / 32);

				Gfx::SetParameter(prefilterShader, "g_PreFilteredMap", prefilterMap, level);
				Gfx::SetParameter(prefilterShader, "roughness", level * deltaRoughness);
				Gfx::Dispatch(numGroups, numGroups, 6);
			}

			Gfx::DestroyShader(prefilterShader);
		}

		// Compute BRDF LUT map
		{
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
		}
	}

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

	int tonemapMode = 1;
	const char* tonemapModes[] = {
		"None",
		"Aces Film",
		"Reinhard"
	};

	// Debug views
	std::array<const char*, 7> debug_views = { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive" };
	int selected_debug_view = 0;

	float3 lightPosition(-1.0f, 1.0f, 2.0f);
	float3 lightColor(1.0f);

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
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load Scene"))
				{
					std::vector<wchar_t*> results;
					if (Utils::OpenFileDialog(window, L"Choose scene to load", results, L"", { L"*.gltf; *.glb"}))
					{
						char path[MAX_PATH];
						Utils::StringConvert(results[0], path);
						scenes.emplace_back(Gfx::Scene::Load(path));
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

				if (ImGui::MenuItem("Take GPU Capture"))
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
#pragma endregion UI

		Gfx::BeginEvent("Geometry Pass");
		Gfx::BindShader(deferredShader);
		Gfx::SetParameter(deferredShader, "viewProj", camera.ViewProj);
		Gfx::SetParameter(deferredShader, "LinearWrap", linearWrapSampler);
		for (Gfx::Scene* scene : scenes)
		{
			scene->DrawMesh([&](const Gfx::Mesh& mesh)
			{
				const Gfx::MeshMaterial& material = scene->GetMaterial(mesh.MaterialID);

				Gfx::SetParameter(deferredShader, "g_albedoTexture", material.Albedo);
				Gfx::SetParameter(deferredShader, "g_roughnessMetalTexture", material.RoughnessMetal);
				Gfx::SetParameter(deferredShader, "g_emissiveTexture", material.Emissive);
				Gfx::SetParameter(deferredShader, "model", mesh.Transform);

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
		Gfx::SetParameter(skyboxShader, "LinearWrap", linearWrapSampler);
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
		Gfx::SetParameter(pbrShader, "LinearClamp", linearClampSampler);
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
		Gfx::SetParameter(compositeShader, "LinearWrap", linearWrapSampler);
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
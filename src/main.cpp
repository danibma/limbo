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

	// Deferred shader
	Gfx::Handle<Gfx::Shader> deferredShader = Gfx::CreateShader({
		.ProgramName = "deferredshading",
		.RTFormats = {
			{ Gfx::Format::RGBA8_UNORM, "Albedo"},
			{ Gfx::Format::RGBA8_UNORM, "Normal" },
			{ Gfx::Format::RGBA8_UNORM, "Roughness" },
			{ Gfx::Format::RGBA8_UNORM, "Metallic" },
			{ Gfx::Format::RGBA8_UNORM, "Emissive" }
		},
		.DepthFormat = { Gfx::Format::D32_SFLOAT },
		.Type = Gfx::ShaderType::Graphics
	});

	// transform an equirectangular map into a cubemap
	Gfx::Handle<Gfx::Texture> environmentCubemap;
	{
		Gfx::Handle<Gfx::Texture> equirectangularTexture = Gfx::CreateTextureFromFile("assets/environment/a_rotes_rathaus_4k.hdr", "Equirectangular Texture");
		environmentCubemap = Gfx::CreateTexture({
			.Width = 1024,
			.Height = 1024,
			.DebugName = "Environment Cubemap",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Gfx::Format::RGBA16_SFLOAT,
			.Type = Gfx::TextureType::TextureCube,
		});

		Gfx::Handle<Gfx::Shader> equirectToCubemap = Gfx::CreateShader({
			.ProgramName = "sky",
			.CsEntryPoint = "EquirectToCubemap",
			.Type = Gfx::ShaderType::Compute
		});

		Gfx::BindShader(equirectToCubemap);
		Gfx::SetParameter(equirectToCubemap, "g_EnvironmentEquirectangular", equirectangularTexture);
		Gfx::SetParameter(equirectToCubemap, "g_OutEnvironmentCubemap", environmentCubemap);
		Gfx::SetParameter(equirectToCubemap, "LinearWrap", linearWrapSampler);
		Gfx::Dispatch(1024 / 32, 1024 / 32, 6);

		Gfx::DestroyTexture(equirectangularTexture);
		Gfx::DestroyShader(equirectToCubemap);
	}

	// Skybox
	Gfx::Handle<Gfx::Shader> skyboxShader;
	Gfx::Scene* skyboxCube = Gfx::Scene::Load("assets/models/skybox.glb");
	{
		skyboxShader = Gfx::CreateShader({
			.ProgramName = "sky",
			.RTFormats = {
				{ Gfx::Format::RGBA8_UNORM, "Skybox RT" }
			},
			.Type = Gfx::ShaderType::Graphics
		});
	}

	// Composite shader
	Gfx::Handle<Gfx::Shader> compositeShader = Gfx::CreateShader({
		.ProgramName = "scenecomposite",
		.Type = Gfx::ShaderType::Graphics
	});

	std::vector<Gfx::Scene*> scenes;

	int tonemapMode = 0;
	const char* tonemapModes[] = {
		"None",
		"Aces Film",
		"Reinhard"
	};

	Core::Timer deltaTimer;
	while (!window->ShouldClose())
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		window->PollEvents();

		Gfx::UpdateCamera(window, camera, deltaTime);

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
				ImGui::PopItemWidth();

				if (ImGui::MenuItem("Take GPU Capture"))
					Gfx::TakeGPUCapture();

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

		Gfx::BeginEvent("Scene Composite");
		Gfx::BindShader(compositeShader);
		Gfx::SetParameter(compositeShader, "g_TonemapMode", tonemapMode);
		Gfx::SetParameter(compositeShader, "g_sceneTexture", skyboxShader, 0);
		Gfx::SetParameter(compositeShader, "LinearWrap", linearWrapSampler);
		Gfx::Draw(6);
		Gfx::EndEvent();

		Gfx::Present();
	}

	Gfx::DestroyTexture(environmentCubemap);

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
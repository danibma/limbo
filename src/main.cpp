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

	Gfx::Handle<Gfx::Shader> deferredShader = Gfx::CreateShader({
		.ProgramName = "deferredshading",
		.RTFormats = {
			Gfx::Format::RGBA8_UNORM,
			Gfx::Format::RGBA8_UNORM,
			Gfx::Format::RGBA8_UNORM,
			Gfx::Format::RGBA8_UNORM
		},
		.DepthFormat = Gfx::Format::D32_SFLOAT,
		.Type = Gfx::ShaderType::Graphics
	});

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
					ImGui::DragFloat("Speed", &camera.CameraSpeed, 0.1f, 0.1f);
					ImGui::EndMenu();
				}
				ImGui::Separator();
				ImGui::Combo("Tonemap", &tonemapMode, tonemapModes, 3);
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

		Gfx::BeginEvent("Scene Composite");
		Gfx::BindShader(compositeShader);
		Gfx::SetParameter(compositeShader, "g_TonemapMode", tonemapMode);
		Gfx::SetParameter(compositeShader, "g_sceneTexture", deferredShader, 0);
		Gfx::SetParameter(compositeShader, "LinearWrap", linearWrapSampler);
		Gfx::Draw(6);
		Gfx::EndEvent();

		Gfx::Present();
	}

	Gfx::DestroyShader(deferredShader);
	Gfx::DestroyShader(compositeShader);

	for (Gfx::Scene* scene : scenes)
		Gfx::DestroyScene(scene);

	Gfx::Shutdown();
	Core::DestroyWindow(window);

	return 0;
}
#include "stdafx.h"
#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/scene.h"
#include "gfx/fpscamera.h"

#include "core/window.h"

#include "tests/tests.h"

#include <CLI11/CLI11.hpp>

#include "core/timer.h"

using namespace limbo;

#define WIDTH  1280
#define HEIGHT 720

int main(int argc, char* argv[])
{
	CLI::App app { "limbo" };

	bool bRunTests = false;
	app.add_flag("--tests", bRunTests, "Run tests");
	bool bComputeTriangle = false;
	app.add_flag("--ctriangle", bComputeTriangle, "Run compute triangle test");
	bool bGraphicsTriangle = false;
	app.add_flag("--gtriangle", bGraphicsTriangle, "Run graphics triangle test");

	CLI11_PARSE(app, argc, argv);

	if (bRunTests)
		return tests::executeTests(argc, argv);
	if (bComputeTriangle)
		return tests::executeComputeTriangle();
	if (bGraphicsTriangle)
		return tests::executeGraphicsTriangle();

	core::Window* window = core::createWindow({
		.title = "limbo",
		.width = WIDTH,
		.height = HEIGHT
	});

	gfx::init(window, gfx::GfxDeviceFlag::EnableImgui);

	gfx::FPSCamera camera = gfx::createCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

	gfx::Handle<gfx::Sampler> linearWrapSampler = gfx::createSampler({
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
		.MinLOD = 0.0f,
		.MaxLOD = 1.0f
		});

	gfx::Handle<gfx::Shader> deferredShader = gfx::createShader({
		.programName = "deferredshading",
		.rtFormats = {
			gfx::Format::RGBA8_UNORM,
			gfx::Format::RGBA8_UNORM,
			gfx::Format::RGBA8_UNORM,
			gfx::Format::RGBA8_UNORM
		},
		.depthFormat = gfx::Format::D32_SFLOAT,
		.type = gfx::ShaderType::Graphics
	});

	gfx::Handle<gfx::Shader> compositeShader = gfx::createShader({
		.programName = "scenecomposite",
		.type = gfx::ShaderType::Graphics
	});

	gfx::Scene* scene = gfx::loadScene("models/DamagedHelmet/DamagedHelmet.gltf");
	//gfx::Scene* scene = gfx::loadScene("models/Sponza/Sponza.gltf");

	int tonemapMode = 0;
	const char* tonemapModes[] = {
		"None",
		"Aces Film",
		"Reinhard"
	};

	core::Timer deltaTimer;
	while (!window->shouldClose())
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		window->pollEvents();

		gfx::updateCamera(window, camera, deltaTime);

		ImGui::Begin("Limbo", nullptr);
		ImGui::Text("Selected device: %s", gfx::getGPUInfo().name);
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Speed", &camera.cameraSpeed, 0.1f, 0.1f);
		}
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Profiling", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Frame Time: %.2f ms (%.2f fps)", deltaTime, 1000.0f / deltaTime);
		}
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Combo("Tonemap", &tonemapMode, tonemapModes, 3);
		}
		ImGui::End();

		gfx::beginEvent("Geometry Pass");
		gfx::bindShader(deferredShader);
		gfx::setParameter(deferredShader, "viewProj", camera.viewProj);
		gfx::setParameter(deferredShader, "LinearWrap", linearWrapSampler);

		scene->drawMesh([&](const gfx::Mesh& mesh)
		{
			gfx::setParameter(deferredShader, "g_albedoTexture", mesh.material.albedo);
			gfx::setParameter(deferredShader, "g_roughnessMetalTexture", mesh.material.roughnessMetal);
			gfx::setParameter(deferredShader, "g_emissiveTexture", mesh.material.emissive);
			gfx::setParameter(deferredShader, "model", mesh.transform);

			gfx::bindVertexBuffer(mesh.vertexBuffer);
			gfx::bindIndexBuffer(mesh.indexBuffer);
			gfx::drawIndexed((uint32)mesh.indexCount);
		});
		gfx::endEvent();

		gfx::beginEvent("Scene Composite");
		gfx::bindShader(compositeShader);
		gfx::setParameter(compositeShader, "g_TonemapMode", tonemapMode);
		gfx::setParameter(compositeShader, "g_sceneTexture", deferredShader, 0);
		gfx::setParameter(compositeShader, "LinearWrap", linearWrapSampler);
		gfx::draw(6);
		gfx::endEvent();

		gfx::present();
	}

	gfx::destroyShader(deferredShader);
	gfx::destroyShader(compositeShader);

	gfx::destroyScene(scene);

	gfx::shutdown();
	core::destroyWindow(window);

	return 0;
}
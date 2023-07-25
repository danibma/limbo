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

struct Vertex
{
	float position[3];
};

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

	gfx::init(window);

	gfx::FPSCamera camera = gfx::createCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

	Vertex vertices[] = { { -1.0,  1.0, 1.0 },
						  {  1.0,  1.0, 1.0 },
						  {  0.0, -1.0, 1.0 } };
	gfx::Handle<gfx::Buffer> vertexBuffer = gfx::createBuffer({ 
		.debugName = "triangle vb",
		.byteStride = sizeof(Vertex),
		.byteSize = sizeof(Vertex) * 3,
		.usage = gfx::BufferUsage::Vertex, 
		.initialData = vertices
	});

	gfx::Handle<gfx::Shader> triangleShader = gfx::createShader({
		.programName = "triangle",
		.depthFormat = gfx::Format::D32_SFLOAT,
		.type = gfx::ShaderType::Graphics
	});

	gfx::Handle<gfx::Sampler> linearWrapSampler = gfx::createSampler({
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
		.MinLOD = 0.0f,
		.MaxLOD = 1.0f
	});

	//gfx::Scene* scene = gfx::loadScene("models/DamagedHelmet.glb");
	gfx::Scene* scene = gfx::loadScene("models/Sponza/Sponza.gltf");

	core::Timer deltaTimer;
	for (float time = 0.0f; !window->shouldClose(); time += 0.1f)
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		window->pollEvents();
		Noop(time); // remove warning
		gfx::updateCamera(window, camera, deltaTime);

		float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };

		gfx::bindShader(triangleShader);
		gfx::setParameter(triangleShader, "viewProj", camera.viewProj);
		gfx::setParameter(triangleShader, "color", color);
		gfx::setParameter(triangleShader, "LinearWrap", linearWrapSampler);

		scene->drawMesh([&](const gfx::Mesh& mesh)
		{
			gfx::setParameter(triangleShader, "g_diffuseTexture", mesh.material.diffuse);
			gfx::setParameter(triangleShader, "model", mesh.transform);

			gfx::bindVertexBuffer(mesh.vertexBuffer);
			gfx::bindIndexBuffer(mesh.indexBuffer);
			gfx::drawIndexed((uint32)mesh.indexCount);
		});

		gfx::present();
	}

	gfx::destroyBuffer(vertexBuffer);
	gfx::destroyShader(triangleShader);

	gfx::destroyScene(scene);

	gfx::shutdown();
	core::destroyWindow(window);

	return 0;
}
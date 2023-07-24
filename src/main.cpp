#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/draw.h"
#include "gfx/scene.h"

#include "core/fpscamera.h"

#include "tests/tests.h"

#include <CLI11/CLI11.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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
	else if (bComputeTriangle)
		return tests::executeComputeTriangle();
	else if (bGraphicsTriangle)
		return tests::executeGraphicsTriangle();

	if (!glfwInit())
	{
		LB_ERROR("Failed to initialize GLFW!");
		return 0;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "limbo", nullptr, nullptr);
	if (!window)
	{
		LB_ERROR("Failed to create GLFW window!");
		return 0;
	}
	
	gfx::init({ 
		.hwnd = glfwGetWin32Window(window),
		.width = WIDTH,
		.height = HEIGHT,
	});

	core::FPSCamera camera = core::CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

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
		.type = gfx::ShaderType::Graphics
	});

	gfx::Scene* damagedHelmet = gfx::loadScene("models/DamagedHelmet.glb");

	core::Timer deltaTimer;
	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		glfwPollEvents();
		Noop(time); // remove warning
		core::UpdateCamera(window, camera, deltaTime);

		float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };
		gfx::setParameter(triangleShader, "viewProj", camera.viewProj);
		gfx::setParameter(triangleShader, "model", float4x4(1.0f));
		gfx::setParameter(triangleShader, "color", color);
		gfx::bindDrawState({
			.shader = triangleShader,
			.viewport = {
				.Width = WIDTH,
				.Height = HEIGHT,
				.MaxDepth = 1
			},
			.scissor = {
				.right = WIDTH,
				.bottom = HEIGHT
			}
		});
		//gfx::bindVertexBuffer(vertexBuffer);
		//gfx::draw(3);
		damagedHelmet->drawMesh([&](const gfx::Mesh& mesh)
		{
			gfx::bindVertexBuffer(mesh.vertexBuffer);
			gfx::bindIndexBuffer(mesh.indexBuffer);
			gfx::drawIndexed((uint32)mesh.indexCount);
		});

		gfx::present();
	}

	gfx::destroyBuffer(vertexBuffer);
	gfx::destroyShader(triangleShader);

	gfx::destroyScene(damagedHelmet);

	gfx::shutdown();
	glfwTerminate();

	return 0;
}
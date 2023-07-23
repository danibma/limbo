#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/draw.h"

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

	CLI11_PARSE(app, argc, argv);

	if (bRunTests)
		return tests::executeTests(argc, argv);

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

	D3D12_VIEWPORT swapchainViewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = WIDTH,
		.Height = HEIGHT,
		.MinDepth = 0,
		.MaxDepth = 1
	};

	D3D12_RECT swapchainScissor = {
		.left = 0,
		.top = 0,
		.right = WIDTH,
		.bottom = HEIGHT
	};

#define COMPUTE 0
#if COMPUTE
	gfx::Handle<gfx::Texture> outputTexture = gfx::createTexture({
		.width = WIDTH,
		.height = HEIGHT,
		.debugName = "triangle output texture",
		.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		.format = gfx::Format::RGBA8_UNORM,
		.type = gfx::TextureType::Texture2D
	});

	gfx::Handle<gfx::Shader> triangleShader = gfx::createShader({
		.programName = "compute_triangle",
		.cs_entryPoint = "DrawTriangle",
		.type = gfx::ShaderType::Compute
	});
#else
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
#endif

	core::Timer deltaTimer;
	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		glfwPollEvents();
		Noop(time); // remove warning
		core::UpdateCamera(window, camera, deltaTime);

#if COMPUTE
		gfx::setParameter(triangleShader, "output", outputTexture);
		gfx::bindDrawState({
			.shader = triangleShader,
		});
		gfx::dispatch(WIDTH / 8, HEIGHT / 8, 1);

		gfx::copyTextureToBackBuffer(outputTexture);

		gfx::present();
#else
		float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };
		gfx::setParameter(triangleShader, "viewProj", camera.viewProj);
		gfx::setParameter(triangleShader, "model", float4x4(1.0f));
		gfx::setParameter(triangleShader, "color", color);
		gfx::bindDrawState({
			.shader = triangleShader,
			.viewport = swapchainViewport,
			.scissor = swapchainScissor
		});
		gfx::bindVertexBuffer(vertexBuffer);
		
		gfx::draw(3);
		gfx::present();
#endif
	}

#if COMPUTE
	gfx::destroyTexture(outputTexture);
	gfx::destroyShader(triangleShader);
#else
	gfx::destroyBuffer(vertexBuffer);
	gfx::destroyShader(triangleShader);
#endif

	gfx::shutdown();
	glfwTerminate();

	return 0;
}
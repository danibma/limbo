#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/draw.h"

#include "tests/tests.h"

#include <CLI11/CLI11.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using namespace limbo;

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

	GLFWwindow* window = glfwCreateWindow(1280, 720, "limbo", nullptr, nullptr);
	if (!window)
	{
		LB_ERROR("Failed to create GLFW window!");
		return 0;
	}
	
	gfx::init({ 
		.hwnd = glfwGetWin32Window(window),
		.width = 1280,
		.height = 720,
	});

#define COMPUTE 0
#if COMPUTE
	gfx::Handle<gfx::Texture> outputTexture = gfx::createTexture({
		.width = 1280,
		.height = 720,
		.debugName = "triangle output texture",
		.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		.format = gfx::Format::RGBA8_UNORM,
		.type = gfx::TextureType::Texture2D
	});

	gfx::Handle<gfx::BindGroup> triangleBind = gfx::createBindGroup({
		.textures = {
			{ .slot = 0, .texture = outputTexture }
		},
	});

	gfx::Kernel triangleKernelCS;
	FAILIF(!gfx::ShaderCompiler::compile(triangleKernelCS, "compute_triangle", "DrawTriangle", gfx::KernelType::Compute), -1);

	gfx::Handle<gfx::Shader> triangleShader = gfx::createShader({
		.cs = triangleKernelCS,
		.bindGroup = triangleBind,
		.type = gfx::ShaderType::Compute
	});
#else
	float vertices[] = {  0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	gfx::Handle<gfx::Buffer> vertexBuffer = gfx::createBuffer({ 
		.debugName = "triangle vb", 
		.byteSize = sizeof(vertices), 
		.usage = gfx::BufferUsage::Vertex, 
		.initialData = vertices });

	gfx::Handle<gfx::BindGroup> triangleBind = gfx::createBindGroup({
		.inputLayout = {
			{ .semanticName = "Position", .format = gfx::Format::RGB32_SFLOAT }
		}
	});

	gfx::Kernel triangleKernelVS;
	FAILIF(!gfx::ShaderCompiler::compile(triangleKernelVS, "triangle", "VSMain", gfx::KernelType::Vertex), -1);
	gfx::Kernel triangleKernelPS;
	FAILIF(!gfx::ShaderCompiler::compile(triangleKernelPS, "triangle", "PSMain", gfx::KernelType::Pixel), -1);

	gfx::Handle<gfx::Shader> triangleShader = gfx::createShader({
		.vs = triangleKernelVS,
		.ps = triangleKernelPS,
		.rtCount = 1,
		.rtFormats = { gfx::getSwapchainFormat() },
		.bindGroup = triangleBind,
		.type = gfx::ShaderType::Graphics
	});
#endif

	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		glfwPollEvents();
		Noop(time);

#if COMPUTE
		gfx::bindDrawState({
			.shader = triangleShader,
			.bindGroups = { triangleBind },
		});
		gfx::dispatch(1280 / 8, 720 / 8, 1);

		gfx::copyTextureToBackBuffer(outputTexture);

		gfx::present();
#else
		//float color[] = { 0.5f * cosf(time) + 0.5f,
		//				  0.5f * sinf(time) + 0.5f,
		//				  1.0f };
		//
		//setParameter(triangleShader, 0, color);
		gfx::bindDrawState({
			.shader = triangleShader,
			.bindGroup = triangleBind,
			});
		gfx::bindVertexBuffer(vertexBuffer);
		
		gfx::draw(3);
		gfx::present();
#endif
	}

#if COMPUTE
	gfx::destroyTexture(outputTexture);
	gfx::destroyShader(triangleShader);
	gfx::destroyBindGroup(triangleBind);
#else
	gfx::destroyBuffer(vertexBuffer);
	gfx::destroyShader(triangleShader);
	gfx::destroyBindGroup(triangleBind);
#endif

	gfx::shutdown();
	glfwTerminate();

	return 0;
}
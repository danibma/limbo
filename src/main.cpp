#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/draw.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using namespace limbo;

int main(int argc, char* argv[])
{
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

#define COMPUTE 1
#if COMPUTE
	Handle<gfx::Texture> outputTexture = gfx::createTexture({
		.width = 1280,
		.height = 720,
		.debugName = "triangle output texture",
		.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		.format = gfx::Format::RGBA8_UNORM,
		.type = gfx::TextureType::Texture2D
	});

	Handle<gfx::BindGroup> triangleBind = gfx::createBindGroup({
		.textures = {
			{ .slot = 0, .texture = outputTexture }
		},
	});

	Handle<gfx::Shader> triangleCSShader = gfx::createShader({
		.programName = "compute_triangle",
		.entryPoint = "DrawTriangle",
		.bindGroups = { triangleBind },
		.type = gfx::ShaderType::Compute
	});
#else
	float vertices[] = {  0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	Handle<Buffer> vertexBuffer = createBuffer({ 
		.debugName = "triangle vb", 
		.byteSize = 150, 
		.usage = BufferUsage::Vertex, 
		.initialData = vertices });

	Handle<Shader> triangleShader = createShader({});
#endif

	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		glfwPollEvents();
		Noop(time);

#if COMPUTE
		gfx::bindDrawState({
			.shader = triangleCSShader,
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
		//bindDrawState({
		//	.shader = triangleShader,
		//	//.bindGroups = { triangleBind },
		//	});
		//bindVertexBuffer(vertexBuffer);
		//
		//draw(3);
		present();
#endif
	}

#if COMPUTE
	gfx::destroyTexture(outputTexture);
	gfx::destroyShader(triangleCSShader);
	gfx::destroyBindGroup(triangleBind);
#else
	gfx::destroyBuffer(vertexBuffer);
	gfx::destroyShader(triangleShader);
#endif

	gfx::shutdown();
	glfwTerminate();

	return 0;
}
#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace limbo::tests::gfx
{
	int runComputeTriangle()
	{
		constexpr uint32 WIDTH = 1280;
		constexpr uint32 HEIGHT = 720;

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

		limbo::gfx::init({
			.hwnd = glfwGetWin32Window(window),
			.width = WIDTH,
			.height = HEIGHT,
			});

		limbo::gfx::Handle<limbo::gfx::Texture> outputTexture = limbo::gfx::createTexture({
			.width = WIDTH,
			.height = HEIGHT,
			.debugName = "triangle output texture",
			.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.format = limbo::gfx::Format::RGBA8_UNORM,
			.type = limbo::gfx::TextureType::Texture2D
			});

		limbo::gfx::Handle<limbo::gfx::Shader> triangleShader = limbo::gfx::createShader({
			.programName = "../src/tests/gfx/shaders/compute_triangle",
			.cs_entryPoint = "DrawTriangle",
			.type = limbo::gfx::ShaderType::Compute
			});


		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			limbo::gfx::setParameter(triangleShader, "output", outputTexture);
			limbo::gfx::bindShader(triangleShader);
			limbo::gfx::dispatch(WIDTH / 8, HEIGHT / 8, 1);

			limbo::gfx::copyTextureToBackBuffer(outputTexture);

			limbo::gfx::present();
		}

		limbo::gfx::destroyTexture(outputTexture);
		limbo::gfx::destroyShader(triangleShader);

		limbo::gfx::shutdown();
		glfwTerminate();

		return 0;
	}
}
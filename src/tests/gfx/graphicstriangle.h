#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/draw.h"

#include "core/fpscamera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "core/timer.h"

namespace limbo::tests::gfx
{
	int runGfxTriangle()
	{
		struct Vertex
		{
			float position[3];
		};

		constexpr uint32 WIDTH  = 1280;
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

		core::FPSCamera camera = core::CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

		Vertex vertices[] = { { -1.0,  1.0, 1.0 },
							  {  1.0,  1.0, 1.0 },
							  {  0.0, -1.0, 1.0 } };
		limbo::gfx::Handle<limbo::gfx::Buffer> vertexBuffer = limbo::gfx::createBuffer({
			.debugName = "triangle vb",
			.byteStride = sizeof(Vertex),
			.byteSize = sizeof(Vertex) * 3,
			.usage = limbo::gfx::BufferUsage::Vertex,
			.initialData = vertices
			});

		limbo::gfx::Handle<limbo::gfx::Shader> triangleShader = limbo::gfx::createShader({
			.programName = "../src/tests/gfx/shaders/triangle",
			.type = limbo::gfx::ShaderType::Graphics
			});

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
			limbo::gfx::setParameter(triangleShader, "viewProj", camera.viewProj);
			limbo::gfx::setParameter(triangleShader, "model", float4x4(1.0f));
			limbo::gfx::setParameter(triangleShader, "color", color);
			limbo::gfx::bindDrawState({
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
			limbo::gfx::bindVertexBuffer(vertexBuffer);

			limbo::gfx::draw(3);
			limbo::gfx::present();
		}

		limbo::gfx::destroyBuffer(vertexBuffer);
		limbo::gfx::destroyShader(triangleShader);

		limbo::gfx::shutdown();
		glfwTerminate();
		return 0;
	}
}
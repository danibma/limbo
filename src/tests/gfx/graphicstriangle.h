#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#include "gfx/fpscamera.h"

#include "core/window.h"

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

		core::Window* window = core::createWindow({
			.title = "limbo -> graphics triangle test",
			.width = WIDTH,
			.height = HEIGHT
		});

		limbo::gfx::init(window);

		limbo::gfx::FPSCamera camera = limbo::gfx::createCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

		Vertex vertices[] = { {  0.0,  1.0, -1.0 },
							  { -1.0, -1.0, -1.0 },
							  {  1.0, -1.0, -1.0 } };
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
		for (float time = 0.0f; !window->shouldClose(); time += 0.1f)
		{
			float deltaTime = deltaTimer.ElapsedMilliseconds();
			deltaTimer.Record();

			window->pollEvents();
			Noop(time); // remove warning
			limbo::gfx::updateCamera(window, camera, deltaTime);

			float color[] = { 0.5f * cosf(time) + 0.5f,
							  0.5f * sinf(time) + 0.5f,
							  1.0f };
			limbo::gfx::setParameter(triangleShader, "viewProj", camera.viewProj);
			limbo::gfx::setParameter(triangleShader, "model", float4x4(1.0f));
			limbo::gfx::setParameter(triangleShader, "color", color);
			limbo::gfx::bindShader(triangleShader);
			limbo::gfx::bindVertexBuffer(vertexBuffer);

			limbo::gfx::draw(3);
			limbo::gfx::present();
		}

		limbo::gfx::destroyBuffer(vertexBuffer);
		limbo::gfx::destroyShader(triangleShader);

		limbo::gfx::shutdown();
		core::destroyWindow(window);

		return 0;
	}
}
#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#include "gfx/fpscamera.h"

#include "core/window.h"

#include "core/timer.h"

namespace limbo::Tests::Gfx
{
	int RunGfxTriangle()
	{
		struct Vertex
		{
			float Position[3];
		};

		constexpr uint32 WIDTH  = 1280;
		constexpr uint32 HEIGHT = 720;

		Core::Window* window = Core::NewWindow({
			.Title = "limbo -> graphics triangle test",
			.Width = WIDTH,
			.Height = HEIGHT
		});

		limbo::Gfx::Init(window);

		limbo::Gfx::FPSCamera camera = limbo::Gfx::CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

		Vertex vertices[] = { {  0.0,  1.0, -1.0 },
							  { -1.0, -1.0, -1.0 },
							  {  1.0, -1.0, -1.0 } };
		limbo::Gfx::Handle<limbo::Gfx::Buffer> vertexBuffer = limbo::Gfx::CreateBuffer({
			.DebugName = "triangle vb",
			.ByteStride = sizeof(Vertex),
			.ByteSize = sizeof(Vertex) * 3,
			.Usage = limbo::Gfx::BufferUsage::Vertex,
			.InitialData = vertices
		});

		limbo::Gfx::Handle<limbo::Gfx::Shader> triangleShader = limbo::Gfx::CreateShader({
			.ProgramName = "../src/tests/gfx/shaders/triangle",
			.Type = limbo::Gfx::ShaderType::Graphics
			});

		Core::Timer deltaTimer;
		for (float time = 0.0f; !window->ShouldClose(); time += 0.1f)
		{
			float deltaTime = deltaTimer.ElapsedMilliseconds();
			deltaTimer.Record();

			window->PollEvents();
			Noop(time); // remove warning
			limbo::Gfx::UpdateCamera(window, camera, deltaTime);

			float color[] = { 0.5f * cosf(time) + 0.5f,
							  0.5f * sinf(time) + 0.5f,
							  1.0f };
			limbo::Gfx::SetParameter(triangleShader, "viewProj", camera.ViewProj);
			limbo::Gfx::SetParameter(triangleShader, "model", float4x4(1.0f));
			limbo::Gfx::SetParameter(triangleShader, "color", color);
			limbo::Gfx::BindShader(triangleShader);
			limbo::Gfx::BindVertexBuffer(vertexBuffer);

			limbo::Gfx::Draw(3);
			limbo::Gfx::Present(true);
		}

		limbo::Gfx::DestroyBuffer(vertexBuffer);
		limbo::Gfx::DestroyShader(triangleShader);

		limbo::Gfx::Shutdown();
		Core::DestroyWindow(window);

		return 0;
	}
}
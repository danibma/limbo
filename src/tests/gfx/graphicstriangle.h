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

		limbo::RHI::Init(window);

		limbo::RHI::FPSCamera camera = limbo::RHI::CreateCamera(float3(0.0f, 0.0f, 5.0f), float3(0.0f, 0.0f, -1.0f));

		Vertex vertices[] = { {  0.0,  1.0, -1.0 },
							  { -1.0, -1.0, -1.0 },
							  {  1.0, -1.0, -1.0 } };
		limbo::RHI::Handle<limbo::RHI::Buffer> vertexBuffer = limbo::RHI::CreateBuffer({
			.DebugName = "triangle vb",
			.ByteStride = sizeof(Vertex),
			.ByteSize = sizeof(Vertex) * 3,
			.Flags = limbo::RHI::BufferUsage::Vertex,
			.InitialData = vertices
		});

		limbo::RHI::Handle<limbo::RHI::Shader> triangleShader = limbo::RHI::CreateShader({
			.ProgramName = "../src/tests/gfx/shaders/triangle",
			.Type = limbo::RHI::ShaderType::Graphics
			});

		Core::Timer deltaTimer;
		for (float time = 0.0f; !window->ShouldClose(); time += 0.1f)
		{
			float deltaTime = deltaTimer.ElapsedMilliseconds();
			deltaTimer.Record();

			window->PollEvents();
			Noop(time); // remove warning
			limbo::RHI::UpdateCamera(window, camera, deltaTime);

			float color[] = { 0.5f * cosf(time) + 0.5f,
							  0.5f * sinf(time) + 0.5f,
							  1.0f };
			limbo::RHI::BindAccelerationStructure(triangleShader, "viewProj", camera.ViewProj);
			limbo::RHI::SetParameter(triangleShader, "model", float4x4(1.0f));
			limbo::RHI::SetParameter(triangleShader, "color", color);
			limbo::RHI::BindShader(triangleShader);
			limbo::RHI::BindVertexBuffer(vertexBuffer);

			limbo::RHI::Draw(3);
			limbo::RHI::Present(true);
		}

		limbo::RHI::DestroyBuffer(vertexBuffer);
		limbo::RHI::DestroyShader(triangleShader);

		limbo::RHI::Shutdown();
		Core::DestroyWindow(window);

		return 0;
	}
}
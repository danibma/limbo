#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#include "core/window.h"

namespace limbo::Tests::gfx
{
	int RunComputeTriangle()
	{
		constexpr uint32 WIDTH = 1280;
		constexpr uint32 HEIGHT = 720;

		Core::Window* window = Core::NewWindow({
			.Title = "limbo -> compute triangle test",
			.Width = WIDTH,
			.Height = HEIGHT
		});

		limbo::Gfx::Init(window);

		limbo::Gfx::Handle<limbo::Gfx::Texture> outputTexture = limbo::Gfx::CreateTexture({
			.Width = WIDTH,
			.Height = HEIGHT,
			.DebugName = "triangle output texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = limbo::Gfx::Format::RGBA8_UNORM,
			.Type = limbo::Gfx::TextureType::Texture2D
			});

		limbo::Gfx::Handle<limbo::Gfx::Shader> triangleShader = limbo::Gfx::CreateShader({
			.ProgramName = "../src/tests/gfx/shaders/compute_triangle",
			.CsEntryPoint = "DrawTriangle",
			.Type = limbo::Gfx::ShaderType::Compute
			});


		while (!window->ShouldClose())
		{
			window->PollEvents();

			limbo::Gfx::SetParameter(triangleShader, "output", outputTexture);
			limbo::Gfx::BindShader(triangleShader);
			limbo::Gfx::Dispatch(WIDTH / 8, HEIGHT / 8, 1);

			limbo::Gfx::CopyTextureToBackBuffer(outputTexture);

			limbo::Gfx::Present();
		}

		limbo::Gfx::DestroyTexture(outputTexture);
		limbo::Gfx::DestroyShader(triangleShader);

		limbo::Gfx::Shutdown();
		Core::DestroyWindow(window);

		return 0;
	}
}

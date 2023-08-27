#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#include "core/window.h"

namespace limbo::Tests::Gfx
{
	int RunComputeTriangle()
	{
		Core::Window* window = Core::NewWindow({
			.Title = "limbo -> compute triangle test",
			.Width = 1280,
			.Height = 720,
			.Maximized = false
		});

		limbo::Gfx::Init(window);

		limbo::Gfx::Handle<limbo::Gfx::Texture> outputTexture = limbo::Gfx::CreateTexture({
			.Width = limbo::Gfx::GetBackbufferWidth(),
			.Height = limbo::Gfx::GetBackbufferHeight(),
			.DebugName = "triangle output texture",
			.Flags = limbo::Gfx::TextureUsage::UnorderedAccess,
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
			limbo::Gfx::Dispatch(limbo::Gfx::GetBackbufferWidth() / 8, limbo::Gfx::GetBackbufferHeight() / 8, 1);

			limbo::Gfx::CopyTextureToBackBuffer(outputTexture);

			limbo::Gfx::Present(true);
		}

		limbo::Gfx::DestroyTexture(outputTexture);
		limbo::Gfx::DestroyShader(triangleShader);

		limbo::Gfx::Shutdown();
		Core::DestroyWindow(window);

		return 0;
	}
}

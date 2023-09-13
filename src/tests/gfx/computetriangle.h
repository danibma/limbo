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

		limbo::RHI::Init(window);

		limbo::RHI::Handle<limbo::RHI::Texture> outputTexture = limbo::RHI::CreateTexture({
			.Width = limbo::RHI::GetBackbufferWidth(),
			.Height = limbo::RHI::GetBackbufferHeight(),
			.DebugName = "triangle output texture",
			.Flags = limbo::RHI::TextureUsage::UnorderedAccess,
			.Format = limbo::RHI::Format::RGBA8_UNORM,
			.Type = limbo::RHI::TextureType::Texture2D
		});

		limbo::RHI::Handle<limbo::RHI::Shader> triangleShader = limbo::RHI::CreateShader({
			.ProgramName = "../src/tests/gfx/shaders/compute_triangle",
			.CsEntryPoint = "DrawTriangle",
			.Type = limbo::RHI::ShaderType::Compute
		});


		while (!window->ShouldClose())
		{
			window->PollEvents();

			limbo::RHI::SetParameter(triangleShader, "output", outputTexture);
			limbo::RHI::BindShader(triangleShader);
			limbo::RHI::Dispatch(limbo::RHI::GetBackbufferWidth() / 8, limbo::RHI::GetBackbufferHeight() / 8, 1);

			limbo::RHI::CopyTextureToBackBuffer(outputTexture);

			limbo::RHI::Present(true);
		}

		limbo::RHI::DestroyTexture(outputTexture);
		limbo::RHI::DestroyShader(triangleShader);

		limbo::RHI::Shutdown();
		Core::DestroyWindow(window);

		return 0;
	}
}

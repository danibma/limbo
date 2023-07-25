#pragma once

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/buffer.h"

#include "core/window.h"

namespace limbo::tests::gfx
{
	int runComputeTriangle()
	{
		constexpr uint32 WIDTH = 1280;
		constexpr uint32 HEIGHT = 720;

		core::Window* window = core::createWindow({
			.title = "limbo -> compute triangle test",
			.width = WIDTH,
			.height = HEIGHT
		});

		limbo::gfx::init(window);

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


		while (!window->shouldClose())
		{
			window->pollEvents();

			limbo::gfx::setParameter(triangleShader, "output", outputTexture);
			limbo::gfx::bindShader(triangleShader);
			limbo::gfx::dispatch(WIDTH / 8, HEIGHT / 8, 1);

			limbo::gfx::copyTextureToBackBuffer(outputTexture);

			limbo::gfx::present();
		}

		limbo::gfx::destroyTexture(outputTexture);
		limbo::gfx::destroyShader(triangleShader);

		limbo::gfx::shutdown();
		core::destroyWindow(window);

		return 0;
	}
}

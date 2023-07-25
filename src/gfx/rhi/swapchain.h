#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "texture.h"

namespace limbo::core
{
	class Window;
}

namespace limbo::gfx
{
	struct WindowInfo;

	class Swapchain
	{
		ComPtr<IDXGISwapChain3>			m_swapchain;
		Handle<Texture>					m_backbuffers[NUM_BACK_BUFFERS];
		Handle<Texture>					m_depthBackbuffers[NUM_BACK_BUFFERS];
		const Format					m_format      = Format::RGBA8_UNORM;
		const Format					m_depthFormat = Format::D32_SFLOAT;
		uint32							m_backbufferWidth;
		uint32							m_backbufferHeight;

	public:
		Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, core::Window* window);
		~Swapchain();

		void initBackBuffers();

		void present();

		uint32 getCurrentIndex();

		Handle<Texture> getBackbuffer(uint32 index);
		Handle<Texture> getDepthBackbuffer(uint32 index);
		Format getFormat();
		Format getDepthFormat();

		uint32 getBackbufferWidth();
		uint32 getBackbufferHeight();
	};
}

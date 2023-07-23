#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "texture.h"

namespace limbo::gfx
{
	struct WindowInfo;

	class Swapchain
	{
		ComPtr<IDXGISwapChain3>			m_swapchain;
		Handle<Texture>					m_backbuffers[NUM_BACK_BUFFERS];
		Format							m_format = Format::RGBA8_UNORM;
		uint32							m_backbufferWidth;
		uint32							m_backbufferHeight;

	public:
		Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, const WindowInfo& info);
		~Swapchain();

		void initBackBuffers();

		void present();

		uint32 getCurrentIndex();

		Handle<Texture> getBackbuffer(uint32 index);
		Format getFormat();

		uint32 getBackbufferWidth();
		uint32 getBackbufferHeight();
	};
}

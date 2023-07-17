#pragma once

#include "definitions.h"

namespace limbo::gfx
{
	struct WindowInfo;

	class Swapchain
	{
		ComPtr<IDXGISwapChain3>			m_swapchain;

		Format							m_format = Format::RGBA8_UNORM;

		ComPtr<ID3D12Resource>			m_backbuffers[NUM_BACK_BUFFERS];

	public:
		Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, const WindowInfo& info);
		~Swapchain() = default;

		void present();

		uint32 getCurrentIndex();

		ID3D12Resource* getBackbuffer(uint32 index);
		Format getFormat();
	};
}

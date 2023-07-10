﻿#pragma once

#include "d3d12definitions.h"

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	class D3D12Swapchain
	{
		ComPtr<IDXGISwapChain3>			m_swapchain;

		DXGI_FORMAT						m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		ComPtr<ID3D12Resource>			m_backbuffers[NUM_BACK_BUFFERS];

	public:
		D3D12Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, const WindowInfo& info);
		~D3D12Swapchain() = default;

		void present();

		uint32 getCurrentIndex();

		ID3D12Resource* getBackbuffer(uint32 index);
	};
}

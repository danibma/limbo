﻿#include "d3d12swapchain.h"

#include "limbo.h"

namespace limbo::rhi
{
	D3D12Swapchain::D3D12Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, const WindowInfo& info)
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {
			.Width = info.width,
			.Height = info.height,
			.Format = m_format,
			.Stereo = false,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 3,
			.Scaling = DXGI_SCALING_NONE,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD ,
			.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
			.Flags = 0
		};
		ComPtr<IDXGISwapChain1> tempSwapchain;
		DX_CHECK(factory->CreateSwapChainForHwnd(queue, info.hwnd, &desc, nullptr, nullptr, &tempSwapchain));
		tempSwapchain->QueryInterface(IID_PPV_ARGS(&m_swapchain));
	}

	void D3D12Swapchain::present()
	{
		m_swapchain->Present(1, 0);
	}

	uint32 D3D12Swapchain::getCurrentIndex()
	{
		return m_swapchain->GetCurrentBackBufferIndex();
	}
}

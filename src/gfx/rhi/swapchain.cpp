#include "swapchain.h"
#include "resourcemanager.h"
#include "gfx/gfx.h"

#include <format>

#include "device.h"

namespace limbo::gfx
{
	Swapchain::Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, const WindowInfo& info)
		: m_backbufferWidth(info.width), m_backbufferHeight(info.height)
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {
			.Width = info.width,
			.Height = info.height,
			.Format = d3dFormat(m_format),
			.Stereo = false,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = NUM_BACK_BUFFERS,
			.Scaling = DXGI_SCALING_NONE,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD ,
			.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
			.Flags = 0
		};
		ComPtr<IDXGISwapChain1> tempSwapchain;
		DX_CHECK(factory->CreateSwapChainForHwnd(queue, info.hwnd, &desc, nullptr, nullptr, &tempSwapchain));
		tempSwapchain->QueryInterface(IID_PPV_ARGS(&m_swapchain));
	}

	Swapchain::~Swapchain()
	{
		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
			destroyTexture(m_backbuffers[i]);
	}

	void Swapchain::initBackBuffers()
	{
		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			ID3D12Resource* tempBuffer;
			m_swapchain->GetBuffer(i, IID_PPV_ARGS(&tempBuffer));

			std::string debugName = std::format("swapchain backbuffer({})", i);
			m_backbuffers[i] = createTexture(tempBuffer, {
				.debugName = debugName.c_str(),
				.resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				.format = m_format,
				.type = TextureType::Texture2D
			});

			tempBuffer->Release();
		}
	}

	void Swapchain::present()
	{
		m_swapchain->Present(1, 0);
	}

	uint32 Swapchain::getCurrentIndex()
	{
		return m_swapchain->GetCurrentBackBufferIndex();
	}

	Handle<Texture> Swapchain::getBackbuffer(uint32 index)
	{
		ensure(index < NUM_BACK_BUFFERS);
		return m_backbuffers[index];
	}

	Format Swapchain::getFormat()
	{
		return m_format;
	}

	uint32 Swapchain::getBackbufferWidth()
	{
		return m_backbufferWidth;
	}

	uint32 Swapchain::getBackbufferHeight()
	{
		return m_backbufferHeight;
	}
}

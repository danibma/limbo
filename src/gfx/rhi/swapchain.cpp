#include "stdafx.h"
#include "swapchain.h"
#include "gfx/gfx.h"
#include "resourcemanager.h"

#include "core/window.h"

#include <format>


namespace limbo::RHI
{
	Swapchain::Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, Core::Window* window)
		: m_BackbufferWidth(window->Width), m_BackbufferHeight(window->Height), m_CommandQueue(queue)
		, m_Factory(factory), m_Window(window), m_ResizeWidth(0), m_ResizeHeight(0)
	{
		CreateDXGISwapchain();
	}

	Swapchain::~Swapchain()
	{
		DestroyBackbuffers(false);
	}

	void Swapchain::CreateDXGISwapchain()
	{
		DXGI_SWAP_CHAIN_DESC1 desc = {
			.Width = m_BackbufferWidth,
			.Height = m_BackbufferHeight,
			.Format = D3DFormat(m_Format),
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
		RefCountPtr<IDXGISwapChain1> tempSwapchain;
		DX_CHECK(m_Factory->CreateSwapChainForHwnd(m_CommandQueue, m_Window->GetWin32Handle(), &desc, nullptr, nullptr, tempSwapchain.ReleaseAndGetAddressOf()));
		tempSwapchain->QueryInterface(IID_PPV_ARGS(m_Swapchain.GetAddressOf()));
	}

	void Swapchain::InitBackBuffers()
	{
		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			ID3D12Resource* tempBuffer;
			m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&tempBuffer));

			std::string debugName = std::format("swapchain backbuffer({})", i);
			m_Backbuffers[i] = CreateTexture(tempBuffer, {
				.DebugName = debugName.c_str(),
				.Flags = TextureUsage::RenderTarget,
				.Format = m_Format,
				.Type = TextureType::Texture2D,
			});

			std::string depthDebugName = std::format("swapchain depth backbuffer({})", i);
			m_DepthBackbuffers[i] = CreateTexture({
				.Width = m_BackbufferWidth,
				.Height = m_BackbufferHeight,
				.DebugName = depthDebugName.c_str(),
				.Flags = TextureUsage::DepthStencil,
				.ClearValue = {
					.Format = D3DFormat(m_DepthFormat),
					.DepthStencil = {
						.Depth = 1.0f,
						.Stencil = 0
					}
				},
				.Format = m_DepthFormat,
				.Type = TextureType::Texture2D,
			});

			tempBuffer->Release();
		}
	}

	void Swapchain::DestroyBackbuffers(bool bImmediate)
	{
		for (uint32 i = 0; i < NUM_BACK_BUFFERS; ++i)
		{
			DestroyTexture(m_Backbuffers[i], bImmediate);
			DestroyTexture(m_DepthBackbuffers[i], bImmediate);
		}
	}

	void Swapchain::MarkForResize(uint32 width, uint32 height)
	{
		m_ResizeWidth  = width;
		m_ResizeHeight = height;
	}

	void Swapchain::CheckForResize()
	{
		m_BackbufferWidth = m_ResizeWidth;
		m_BackbufferHeight = m_ResizeHeight;

		m_Swapchain.Reset();

		DestroyBackbuffers(true);
		CreateDXGISwapchain();
		InitBackBuffers();

		Delegates::OnResizedSwapchain.Broadcast(m_BackbufferWidth, m_BackbufferHeight);
	}

	void Swapchain::Present(bool vsync)
	{
		m_Swapchain->Present(vsync, 0);
	}

	uint32 Swapchain::GetCurrentIndex()
	{
		return m_Swapchain->GetCurrentBackBufferIndex();
	}

	TextureHandle Swapchain::GetBackbuffer(uint32 index)
	{
		ensure(index < NUM_BACK_BUFFERS);
		return m_Backbuffers[index];
	}

	TextureHandle Swapchain::GetDepthBackbuffer(uint32 index)
	{
		ensure(index < NUM_BACK_BUFFERS);
		return m_DepthBackbuffers[index];
	}

	Format Swapchain::GetFormat()
	{
		return m_Format;
	}

	Format Swapchain::GetDepthFormat()
	{
		return m_DepthFormat;
	}

	uint32 Swapchain::GetBackbufferWidth()
	{
		return m_BackbufferWidth;
	}

	uint32 Swapchain::GetBackbufferHeight()
	{
		return m_BackbufferHeight;
	}
}

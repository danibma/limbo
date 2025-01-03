﻿#pragma once

#include "definitions.h"
#include "texture.h"

#include <dxgi1_6.h>

namespace limbo::Core
{
	class Window;
}

namespace limbo::RHI
{
	struct WindowInfo;

	class Swapchain
	{
		RefCountPtr<IDXGISwapChain3>	m_Swapchain;
		TextureHandle					m_Backbuffers[gRHIBufferCount];
		TextureHandle					m_DepthBackbuffers[gRHIBufferCount];
		const Format					m_Format      = Format::RGBA8_UNORM;
		const Format					m_DepthFormat = Format::D32_FLOAT;
		uint32							m_BackbufferWidth;
		uint32							m_BackbufferHeight;

		// these three are only pointers to the real objects, do not destroy them, they will be destroyed
		ID3D12CommandQueue*				m_CommandQueue;
		IDXGIFactory2*					m_Factory;
		Core::Window*					m_Window;

		uint32							m_ResizeWidth;
		uint32							m_ResizeHeight;

	public:
		Swapchain(ID3D12CommandQueue* queue, IDXGIFactory2* factory, Core::Window* window);
		void DestroyBackbuffers(bool bImmediate);
		~Swapchain();

		void InitBackBuffers();
		void MarkForResize(uint32 width, uint32 height);
		void CheckForResize();
		void Present(bool vsync);

		uint32 GetCurrentIndex();

		TextureHandle GetBackbuffer(uint32 index);
		TextureHandle GetDepthBackbuffer(uint32 index);
		Format GetFormat();
		Format GetDepthFormat();

		uint32 GetBackbufferWidth();
		uint32 GetBackbufferHeight();

	private:
		void CreateDXGISwapchain();
	};
}

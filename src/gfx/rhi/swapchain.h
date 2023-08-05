﻿#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "texture.h"

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	struct WindowInfo;

	class Swapchain
	{
		ComPtr<IDXGISwapChain3>			m_Swapchain;
		Handle<Texture>					m_Backbuffers[NUM_BACK_BUFFERS];
		Handle<Texture>					m_DepthBackbuffers[NUM_BACK_BUFFERS];
		const Format					m_Format      = Format::RGBA8_UNORM;
		const Format					m_DepthFormat = Format::D32_SFLOAT;
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

		Handle<Texture> GetBackbuffer(uint32 index);
		Handle<Texture> GetDepthBackbuffer(uint32 index);
		Format GetFormat();
		Format GetDepthFormat();

		uint32 GetBackbufferWidth();
		uint32 GetBackbufferHeight();

	private:
		void CreateDXGISwapchain();
	};
}

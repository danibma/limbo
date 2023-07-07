#pragma once

#include "device.h"

#include "d3d12definitions.h"

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	class D3D12Swapchain;
	class D3D12Device final : public Device
	{
		ComPtr<IDXGIFactory2>				m_factory;
		ComPtr<IDXGIAdapter1>				m_adapter;
		ComPtr<ID3D12Device>				m_device;

		ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
		ComPtr<ID3D12CommandList>			m_commandList;
		ComPtr<ID3D12CommandQueue>			m_commandQueue;

		ComPtr<ID3D12Fence>					m_fence;
		HANDLE								m_fenceEvent;
		uint32								m_fenceValues[3];

		D3D12Swapchain*						m_swapchain;
		uint32								m_frameIndex;

	public:
		D3D12Device(const WindowInfo& info);
		virtual ~D3D12Device();

		void copyTextureToBackBuffer(Handle<Texture> texture) override;

		void bindDrawState(const DrawInfo& drawState) override;
		void draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;

		void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

		void present() override;

	private:
		void pickGPU();
		void waitGPU();
	};
}

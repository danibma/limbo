#pragma once

#include "device.h"

#include "d3d12definitions.h"

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	enum class D3D12DescriptorHeapType : uint8;
	struct D3D12DescriptorHandle;
	class D3D12DescriptorHeap;
	class D3D12Swapchain;
	class D3D12Texture;
	class D3D12BindGroup;
	class D3D12Device final : public Device
	{
		ComPtr<IDXGIFactory2>				m_factory;
		ComPtr<IDXGIAdapter1>				m_adapter;
		ComPtr<ID3D12Device>				m_device;

		ComPtr<ID3D12CommandAllocator>		m_commandAllocators[NUM_BACK_BUFFERS];
		ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		ComPtr<ID3D12CommandQueue>			m_commandQueue;

		ComPtr<ID3D12Fence>					m_fence;
		HANDLE								m_fenceEvent;
		uint64								m_fenceValues[3] = { 0, 0, 0 };

		D3D12Swapchain*						m_swapchain;
		uint32								m_frameIndex;

		D3D12DescriptorHeap*				m_srvheap;
		D3D12DescriptorHeap*				m_dsvheap;
		D3D12DescriptorHeap*				m_rtvheap;

		D3D12BindGroup*						m_boundBindGroup;

		std::vector<D3D12_RESOURCE_BARRIER> m_resourceBarriers;

	public:
		D3D12Device(const WindowInfo& info);
		virtual ~D3D12Device();

		virtual void copyTextureToBackBuffer(Handle<Texture> texture) override;

		virtual void bindDrawState(const DrawInfo& drawState) override;
		virtual void draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;

		virtual void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

		virtual void present() override;

		// D3D12 specific
		ID3D12Device* getDevice() const { return m_device.Get(); }

		D3D12DescriptorHandle allocateHandle(D3D12DescriptorHeapType heapType);

		void transitionResource(D3D12Texture* texture, D3D12_RESOURCE_STATES newState);

	private:
		void pickGPU();
		void waitGPU();

		void nextFrame();

		void submitResourceBarriers();
	};
}

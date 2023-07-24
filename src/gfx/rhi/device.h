#pragma once

#include "definitions.h"
#include "resourcepool.h"

#include <vector>

namespace limbo::gfx
{
	enum class DescriptorHeapType : uint8;
	struct DescriptorHandle;
	class DescriptorHeap;
	struct WindowInfo;
	struct DrawInfo;
	class Swapchain;
	class Buffer;
	class Texture;

	class Device
	{
		ComPtr<IDXGIFactory2>				m_factory;
		ComPtr<IDXGIAdapter1>				m_adapter;
		ComPtr<ID3D12Device>				m_device;
		DWORD								m_messageCallbackCookie;

		ComPtr<ID3D12CommandAllocator>		m_commandAllocators[NUM_BACK_BUFFERS];
		ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		ComPtr<ID3D12CommandQueue>			m_commandQueue;

		ComPtr<ID3D12Fence>					m_fence;
		HANDLE								m_fenceEvent;
		uint64								m_fenceValues[3] = { 0, 0, 0 };

		Swapchain*							m_swapchain;
		uint32								m_frameIndex;

		DescriptorHeap*						m_srvheap;
		DescriptorHeap*						m_dsvheap;
		DescriptorHeap*						m_rtvheap;
		DescriptorHeap*						m_samplerheap;

		std::vector<D3D12_RESOURCE_BARRIER> m_resourceBarriers;

	public:
		static Device* ptr;

	public:
		Device(const WindowInfo& info);
		~Device();

		void destroySwapchainBackBuffers();

		void copyTextureToBackBuffer(Handle<Texture> texture);

		void bindVertexBuffer(Handle<Buffer> buffer);
		void bindIndexBuffer(Handle<Buffer> buffer);
		void bindDrawState(const DrawInfo& drawState);
		void draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
		void drawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance);

		void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);

		void present();

		Format getSwapchainFormat();

		// D3D12 specific
		ID3D12Device* getDevice() const { return m_device.Get(); }

		void initSwapchainBackBuffers();

		DescriptorHandle allocateHandle(DescriptorHeapType heapType);

		void transitionResource(Texture* texture, D3D12_RESOURCE_STATES newState);
		void transitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState);
		void copyResource(ID3D12Resource* dst, ID3D12Resource* src);
		void copyBufferToTexture(ID3D12Resource* dst, ID3D12Resource* src);

		uint32 getBackbufferWidth();
		uint32 getBackbufferHeight();

	private:
		void pickGPU();
		void waitGPU();

		void nextFrame();

		void submitResourceBarriers();
	};

	inline void copyTextureToBackBuffer(Handle<Texture> texture)
	{
		Device::ptr->copyTextureToBackBuffer(texture);
	}

	inline void bindVertexBuffer(Handle<Buffer> buffer)
	{
		Device::ptr->bindVertexBuffer(buffer);
	}

	inline void bindIndexBuffer(Handle<Buffer> buffer)
	{
		Device::ptr->bindIndexBuffer(buffer);
	}

	inline void bindDrawState(const DrawInfo&& drawState)
	{
		Device::ptr->bindDrawState(drawState);
	}

	inline void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0)
	{
		Device::ptr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void drawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 baseVertex = 0, uint32 firstInstance = 0)
	{
		Device::ptr->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	inline void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		Device::ptr->dispatch(groupCountX, groupCountY, groupCountZ);
	}

	inline void present()
	{
		Device::ptr->present();
	}

	inline Format getSwapchainFormat()
	{
		return Device::ptr->getSwapchainFormat();
	}

	inline uint32 getBackbufferWidth()
	{
		return Device::ptr->getBackbufferWidth();
	}

	inline uint32 getBackbufferHeight()
	{
		return Device::ptr->getBackbufferHeight();
	}
}

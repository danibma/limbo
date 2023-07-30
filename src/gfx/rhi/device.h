#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "resourcemanager.h"

#include <vector>

namespace limbo::core
{
	class Window;
}

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
	class Shader;

	typedef uint8 GfxDeviceFlags;

	struct GPUInfo
	{
		char name[128];
	};

	class Device
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

		Swapchain*							m_swapchain;
		uint32								m_frameIndex;

		DescriptorHeap*						m_srvheap;
		DescriptorHeap*						m_dsvheap;
		DescriptorHeap*						m_rtvheap;
		DescriptorHeap*						m_samplerheap;

		std::vector<D3D12_RESOURCE_BARRIER> m_resourceBarriers;

		GfxDeviceFlags						m_flags;

		GPUInfo								m_gpuInfo;

		Handle<Shader>						m_boundShader;

	public:
		static Device* ptr;

	public:
		Device(core::Window* window, GfxDeviceFlags flags);
		~Device();

		void copyTextureToBackBuffer(Handle<Texture> texture);
		void copyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst);
		void copyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);
		void copyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);

		template<typename T1, typename T2>
		void copyResource(Handle<T1> src, Handle<T2> dst)
		{
			static_assert(std::_Is_any_of_v<T1, Texture, Buffer>);
			static_assert(std::_Is_any_of_v<T2, Texture, Buffer>);

			Texture* dstResource = ResourceManager::ptr->getTexture(dst);
			FAILIF(!dstResource);
			Buffer* srcResource = ResourceManager::ptr->getBuffer(src);
			FAILIF(!srcResource);

			m_commandList->CopyResource(dstResource->resource.Get(), srcResource->resource.Get());
		}

		void beginEvent(const char* name, uint64 color = 0);
		void endEvent();

		void bindVertexBuffer(Handle<Buffer> buffer);
		void bindIndexBuffer(Handle<Buffer> buffer);
		void bindShader(Handle<Shader> shader);
		void draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
		void drawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance);

		void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);

		void present();

		uint32 getCurrentFrameIndex() const
		{
			return m_frameIndex;
		}

		const GPUInfo& getGPUInfo() const
		{
			return m_gpuInfo;
		}

		Format getSwapchainFormat();
		Format getSwapchainDepthFormat();

		// D3D12 specific
		ID3D12Device* getDevice() const { return m_device.Get(); }
		ID3D12GraphicsCommandList* getCommandList() const { return m_commandList.Get(); }

		DescriptorHandle allocateHandle(DescriptorHeapType heapType);

		void transitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState);
		void transitionResource(Texture* texture, D3D12_RESOURCE_STATES newState);

		void transitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState);
		void transitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState);

		uint32 getBackbufferWidth();
		uint32 getBackbufferHeight();

	private:
		void initSwapchainBackBuffers();
		void destroySwapchainBackBuffers();

		void pickGPU();
		void waitGPU();

		void nextFrame();

		void submitResourceBarriers();

		void installDrawState();
		void bindSwapchainRenderTargets();
	};

	inline const GPUInfo& getGPUInfo()
	{
		return Device::ptr->getGPUInfo();
	}

	inline void copyTextureToBackBuffer(Handle<Texture> texture)
	{
		Device::ptr->copyTextureToBackBuffer(texture);
	}

	inline void copyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset = 0, uint64 dstOffset = 0)
	{
		Device::ptr->copyBufferToBuffer(src, dst, numBytes, srcOffset, dstOffset);
	}

	template<typename T1, typename T2>
	void copyResource(Handle<T1> src, Handle<T2> dst)
	{
		Device::ptr->copyResource(src, dst);
	}

	inline void copyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst)
	{
		Device::ptr->copyBufferToTexture(src, dst);
	}

	inline void beginEvent(const char* name, uint64 color = 0)
	{
		Device::ptr->beginEvent(name, color);
	}

	inline void endEvent()
	{
		Device::ptr->endEvent();
	}

	inline void bindVertexBuffer(Handle<Buffer> buffer)
	{
		Device::ptr->bindVertexBuffer(buffer);
	}

	inline void bindIndexBuffer(Handle<Buffer> buffer)
	{
		Device::ptr->bindIndexBuffer(buffer);
	}

	inline void bindShader(Handle<Shader> shader)
	{
		Device::ptr->bindShader(shader);
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

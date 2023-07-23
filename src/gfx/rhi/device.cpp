#include "device.h"

#include "draw.h"
#include "swapchain.h"
#include "descriptorheap.h"
#include "resourcemanager.h"
#include "memoryallocator.h"

#include <d3d12/d3dx12/d3dx12.h>


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo::gfx
{
	Device::Device(const WindowInfo& info)
	{
		uint32_t dxgiFactoryFlags = 0;

#if !NO_LOG
		{
			ComPtr<ID3D12Debug> debugController;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();

#if LIMBO_DEBUG
			ComPtr<ID3D12Debug1> debugController1;
			DX_CHECK(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)));
			debugController1->SetEnableGPUBasedValidation(true);
#endif

			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (!FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
			{
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
			}
		}
#endif

		DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

		pickGPU();

		DX_CHECK(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device)));

#if !NO_LOG
		// RenderDoc does not support ID3D12InfoQueue1 so do not enable it when running under it
		{
			bool bUnderRenderDoc = false;

			IID renderDocID;
			if (SUCCEEDED(IIDFromString(L"{A7AA6116-9C8D-4BBA-9083-B4D816B71B78}", &renderDocID)))
			{
				ComPtr<IUnknown> renderDoc;
				if (SUCCEEDED(m_device->QueryInterface(renderDocID, &renderDoc)))
				{
					// Running under RenderDoc, so enable capturing mode
					bUnderRenderDoc = true;
					LB_LOG("Running under Render Doc, ID3D12InfoQueue features won't be enabled!");
				}
			}

			if (!bUnderRenderDoc)
			{
				ComPtr<ID3D12InfoQueue> d3d12InfoQueue;
				DX_CHECK(m_device->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue)));
				ComPtr<ID3D12InfoQueue1> d3d12InfoQueue1;
				DX_CHECK(d3d12InfoQueue->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue1)));
				DX_CHECK(d3d12InfoQueue1->RegisterMessageCallback(internal::dxMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_messageCallbackCookie));
			}
		}
#endif

		m_srvheap = new DescriptorHeap(m_device.Get(), DescriptorHeapType::SRV, true);
		m_rtvheap = new DescriptorHeap(m_device.Get(), DescriptorHeapType::RTV);
		m_dsvheap = new DescriptorHeap(m_device.Get(), DescriptorHeapType::DSV);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		m_swapchain = new Swapchain(m_commandQueue.Get(), m_factory.Get(), info);

		m_frameIndex = m_swapchain->getCurrentIndex();

		for (uint8 i = 0; i < NUM_BACK_BUFFERS; ++i)
			DX_CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));

		DX_CHECK(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

		// Create synchronization objects
		{
			DX_CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceValues[m_frameIndex]++;

			// Create an event handler to use for frame synchronization
			m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
			ensure(m_fenceEvent);
		}

		MemoryAllocator::ptr = new MemoryAllocator();
	}

	Device::~Device()
	{
		waitGPU();

		delete MemoryAllocator::ptr;
		MemoryAllocator::ptr = nullptr;

		delete m_srvheap;
		delete m_rtvheap;
		delete m_dsvheap;
	}

	void Device::destroySwapchainBackBuffers()
	{
		delete m_swapchain;
	}

	void Device::copyTextureToBackBuffer(Handle<Texture> texture)
	{
		ResourceManager* rm = ResourceManager::ptr;

		Texture* d3dTexture = rm->getTexture(texture);

		Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
		Texture* backbuffer = rm->getTexture(backBufferHandle);

		transitionResource(d3dTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
		transitionResource(backbuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		submitResourceBarriers();
		m_commandList->CopyResource(backbuffer->resource.Get(), d3dTexture->resource.Get());

		transitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		submitResourceBarriers();
	}

	void Device::bindVertexBuffer(Handle<Buffer> buffer)
	{
		ResourceManager* rm = ResourceManager::ptr;
		Buffer* vb = rm->getBuffer(buffer);
		transitionResource(vb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = vb->resource->GetGPUVirtualAddress(),
			.SizeInBytes = vb->byteSize,
			.StrideInBytes = vb->byteStride
		};
		m_commandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void Device::bindIndexBuffer(Handle<Buffer> buffer)
	{
		ensure(false);
	}

	void Device::bindDrawState(const DrawInfo& drawState)
	{
		ResourceManager* rm = ResourceManager::ptr;
		Shader* pipeline = rm->getShader(drawState.shader);

		ID3D12DescriptorHeap* heaps[] = { m_srvheap->getHeap() };
		m_commandList->SetDescriptorHeaps(1, heaps);

		if (pipeline->type == ShaderType::Compute)
		{
			m_commandList->SetComputeRootSignature(pipeline->rootSignature.Get());
			pipeline->setComputeRootParameters(m_commandList.Get());
		}
		else if (pipeline->type == ShaderType::Graphics)
		{
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_commandList->SetGraphicsRootSignature(pipeline->rootSignature.Get());
			pipeline->setGraphicsRootParameters(m_commandList.Get());

			if (pipeline->useSwapchainRT)
			{
				Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
				Texture* backbuffer = rm->getTexture(backBufferHandle);

				constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				m_commandList->ClearRenderTargetView(backbuffer->handle.cpuHandle, clearColor, 0, nullptr);
				m_commandList->OMSetRenderTargets(1, &backbuffer->handle.cpuHandle, false, nullptr);
			}
			else
			{
				ensure(false);
			}

			m_commandList->RSSetViewports(1, &drawState.viewport);
			m_commandList->RSSetScissorRects(1, &drawState.scissor);
		}

		m_commandList->SetPipelineState(pipeline->pipelineState.Get());
	}

	void Device::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		submitResourceBarriers();
		m_commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void Device::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		submitResourceBarriers();
		m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void Device::submitResourceBarriers()
	{
		if (m_resourceBarriers.empty()) return;
		m_commandList->ResourceBarrier((uint32)m_resourceBarriers.size(), m_resourceBarriers.data());
		m_resourceBarriers.clear();
	}

	void Device::present()
	{
		{
			Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
			Texture* backbuffer = ResourceManager::ptr->getTexture(backBufferHandle);
			transitionResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT);
			submitResourceBarriers();
		}

		DX_CHECK(m_commandList->Close());

		ID3D12CommandList* cmd[1] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, cmd);
		m_swapchain->present();

		nextFrame();

		MemoryAllocator::ptr->flushUploadBuffers();

		DX_CHECK(m_commandAllocators[m_frameIndex]->Reset());
		DX_CHECK(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

		{
			Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
			Texture* backbuffer = ResourceManager::ptr->getTexture(backBufferHandle);
			transitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
			submitResourceBarriers();
		}
	}

	Format Device::getSwapchainFormat()
	{
		return m_swapchain->getFormat();
	}

	void Device::initSwapchainBackBuffers()
	{
		m_swapchain->initBackBuffers();

		Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
		Texture* backbuffer = ResourceManager::ptr->getTexture(backBufferHandle);
		transitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		submitResourceBarriers();
	}

	DescriptorHandle Device::allocateHandle(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV:
			return m_srvheap->allocateHandle();
		case DescriptorHeapType::RTV:
			return m_rtvheap->allocateHandle();
		case DescriptorHeapType::DSV:
			return m_dsvheap->allocateHandle();
		default: 
			return DescriptorHandle();
		}
	}

	void Device::transitionResource(Texture* texture, D3D12_RESOURCE_STATES newState)
	{
		if (texture->currentState == newState) return;
		m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->resource.Get(), texture->currentState, newState));
		texture->currentState = newState;
	}

	void Device::transitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState)
	{
		if (buffer->currentState == newState) return;
		m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(buffer->resource.Get(), buffer->currentState, newState));
		buffer->currentState = newState;
	}

	void Device::copyResource(ID3D12Resource* dst, ID3D12Resource* src)
	{
		m_commandList->CopyResource(dst, src);
	}

	uint32 Device::getBackbufferWidth()
	{
		return m_swapchain->getBackbufferWidth();
	}

	uint32 Device::getBackbufferHeight()
	{
		return m_swapchain->getBackbufferHeight();
	}

	void Device::pickGPU()
	{
		m_adapter = nullptr;

		// Get IDXGIFactory6
		ComPtr<IDXGIFactory6> factory6;
		DX_CHECK(m_factory->QueryInterface(IID_PPV_ARGS(&factory6)));
		for (uint32_t adapterIndex = 0;
			!FAILED(factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			m_adapter->GetDesc1(&desc);

			// Don't select the Basic Render Driver adapter.
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
				break;
		}

		if (!ensure(m_adapter))
			LB_ERROR("Failed to pick an adapter");

		DXGI_ADAPTER_DESC1 desc;
		DX_CHECK(m_adapter->GetDesc1(&desc));
		LB_WLOG("Initiliazed D3D12 on %ls", desc.Description);
	}

	void Device::nextFrame()
	{
		uint64 currentFenceValue = m_fenceValues[m_frameIndex];
		DX_CHECK(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

		m_frameIndex = m_swapchain->getCurrentIndex();

		if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
		{
			DX_CHECK(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_fenceValues[m_frameIndex] = currentFenceValue + 1;
	}

	void Device::waitGPU()
	{
		// Schedule a signal command in the queue
		m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]);
		
		// Wait until the fence has been processed
		m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
		
		// Increment the fence value for the current frame
		m_fenceValues[m_frameIndex]++;
	}
}

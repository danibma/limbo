#include "d3d12device.h"

#include "d3d12descriptorheap.h"
#include "d3d12resourcemanager.h"
#include "d3d12swapchain.h"

#include <d3d12/d3dx12/d3dx12.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo::rhi
{
	D3D12Device::D3D12Device(const WindowInfo& info)
	{
		uint32_t dxgiFactoryFlags = 0;

#if !NO_LOG
		{
			ComPtr<ID3D12Debug> debugController;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();

			ComPtr<ID3D12Debug1> debugController1;
			DX_CHECK(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)));
			debugController1->SetEnableGPUBasedValidation(true);
#if LIMBO_DEBUG
			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (!FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
			{
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
			}
		}
#endif
#endif

		DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

		pickGPU();

		DX_CHECK(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device)));

		m_srvheap = new D3D12DescriptorHeap(m_device.Get(), D3D12DescriptorHeapType::SRV);
		m_rtvheap = new D3D12DescriptorHeap(m_device.Get(), D3D12DescriptorHeapType::RTV);
		m_dsvheap = new D3D12DescriptorHeap(m_device.Get(), D3D12DescriptorHeapType::DSV);

		DX_CHECK(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

		DX_CHECK(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		m_swapchain = new D3D12Swapchain(m_commandQueue.Get(), m_factory.Get(), info);

		m_frameIndex = m_swapchain->getCurrentIndex();

		// Create synchronization objects
		{
			DX_CHECK(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceValues[m_frameIndex]++;

			// Create an event handler to use for frame synchronization
			m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
			ensure(m_fenceEvent);
		}
	}

	D3D12Device::~D3D12Device()
	{
		waitGPU();

		delete m_swapchain;
		delete m_srvheap;
		delete m_rtvheap;
		delete m_dsvheap;
	}

	void D3D12Device::copyTextureToBackBuffer(Handle<Texture> texture)
	{
	}

	void D3D12Device::bindDrawState(const DrawInfo& drawState)
	{
		D3D12ResourceManager* rm = ResourceManager::getAs<D3D12ResourceManager>();
		D3D12Shader* pipeline = rm->getShader(drawState.shader);
		m_boundBindGroup = rm->getBindGroup(drawState.bindGroups[0]);

		if (pipeline->type == ShaderType::Compute)
		{
			m_commandList->SetComputeRootSignature(m_boundBindGroup->rootSignature.Get());
			m_boundBindGroup->setComputeRootParameters(m_commandList.Get());
		}
		else if (pipeline->type == ShaderType::Graphics)
		{
			m_commandList->SetGraphicsRootSignature(m_boundBindGroup->rootSignature.Get());
			m_boundBindGroup->setGraphicsRootParameters(m_commandList.Get());
		}

		m_commandList->SetPipelineState(pipeline->pipelineState.Get());
	}

	void D3D12Device::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		ensure(false);
	}

	void D3D12Device::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		submitResourceBarriers();
		m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void D3D12Device::nextFrame()
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

	void D3D12Device::submitResourceBarriers()
	{
		if (m_resourceBarriers.empty()) return;
		m_commandList->ResourceBarrier((uint32)m_resourceBarriers.size(), m_resourceBarriers.data());
		m_resourceBarriers.clear();
	}

	void D3D12Device::present()
	{
		DX_CHECK(m_commandList->Close());

		ID3D12CommandList* cmd[1] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, cmd);

		m_swapchain->present();

		nextFrame();

		//DX_CHECK(m_commandAllocator->Reset());
		DX_CHECK(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
	}

	D3D12DescriptorHandle D3D12Device::allocateHandle(D3D12DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case D3D12DescriptorHeapType::SRV:
			return m_srvheap->allocateHandle();
		case D3D12DescriptorHeapType::RTV:
			return m_rtvheap->allocateHandle();
		case D3D12DescriptorHeapType::DSV:
			return m_dsvheap->allocateHandle();
		default: 
			return D3D12DescriptorHandle();
		}
	}

	void D3D12Device::transitionResource(D3D12Texture* texture, D3D12_RESOURCE_STATES newState)
	{
		if (texture->currentState == newState) return;
		m_resourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->resource.Get(), texture->currentState, newState));
		texture->currentState = newState;
	}

	void D3D12Device::pickGPU()
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

	void D3D12Device::waitGPU()
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

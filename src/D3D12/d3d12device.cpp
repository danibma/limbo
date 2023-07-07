#include "d3d12device.h"
#include "d3d12swapchain.h"

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
	}

	void D3D12Device::copyTextureToBackBuffer(Handle<Texture> texture)
	{
	}

	void D3D12Device::bindDrawState(const DrawInfo& drawState)
	{
	}

	void D3D12Device::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
	}

	void D3D12Device::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
	}

	void D3D12Device::present()
	{
		m_swapchain->present();
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

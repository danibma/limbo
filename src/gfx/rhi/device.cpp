#include "stdafx.h"
#include "device.h"

#include "core/window.h"
#include "gfx/gfx.h"
#include "swapchain.h"
#include "descriptorheap.h"
#include "ringbufferallocator.h"

#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_glfw.h>

unsigned int DelegateHandle::CURRENT_ID = 0;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo::gfx
{
	Device::Device(core::Window* window, GfxDeviceFlags flags)
		: m_flags(flags), m_gpuInfo()
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

		window->onWindowShouldClose.AddRaw(this, &Device::onWindowClose);

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
		m_samplerheap = new DescriptorHeap(m_device.Get(), DescriptorHeapType::SAMPLERS, true);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		m_swapchain = new Swapchain(m_commandQueue.Get(), m_factory.Get(), window);
		onPostResourceManagerInit.AddRaw(this, &Device::initSwapchainBackBuffers);
		onPreResourceManagerShutdown.AddRaw(this, &Device::destroySwapchainBackBuffers);

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

		ID3D12DescriptorHeap* heaps[] = { m_srvheap->getHeap(), m_samplerheap->getHeap() };
		m_commandList->SetDescriptorHeaps(2, heaps);

		RingBufferAllocator::ptr = new RingBufferAllocator(utils::ToGB(2));

		// ImGui Stuff
		if (flags & GfxDeviceFlag::EnableImgui)
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			DescriptorHandle imguiDescriptor = m_srvheap->allocateHandle();

			ImGui_ImplGlfw_InitForOther(window->getGLFWHandle(), true);
			ImGui_ImplDX12_Init(m_device.Get(), NUM_BACK_BUFFERS, d3dFormat(m_swapchain->getFormat()),
			                    m_srvheap->getHeap(), imguiDescriptor.cpuHandle, imguiDescriptor.gpuHandle);

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	Device::~Device()
	{
		waitGPU();

		delete RingBufferAllocator::ptr;
		RingBufferAllocator::ptr = nullptr;

		delete m_srvheap;
		delete m_rtvheap;
		delete m_dsvheap;
		delete m_samplerheap;

#if !NO_LOG
		if (m_flags & GfxDeviceFlag::DetailedLogging)
		{
			IDXGIDebug1* dxgiDebug;
			DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
			DX_CHECK(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
			dxgiDebug->Release();
		}
#endif

		if (m_flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
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

	void Device::copyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst)
	{
		Texture* dstTexture = ResourceManager::ptr->getTexture(dst);
		FAILIF(!dstTexture);
		Buffer* srcBuffer = ResourceManager::ptr->getBuffer(src);
		FAILIF(!srcBuffer);

		D3D12_RESOURCE_DESC dstDesc = dstTexture->resource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprints[D3D12_REQ_MIP_LEVELS] = {};
		m_device->GetCopyableFootprints(&dstDesc, 0, dstDesc.MipLevels, 0, srcFootprints, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {
			.pResource = srcBuffer->resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = srcFootprints[0]
		};

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {
			.pResource = dstTexture->resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = 0
		};
		m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	void Device::copyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		Buffer* srcBuffer = ResourceManager::ptr->getBuffer(src);
		FAILIF(!srcBuffer);
		Buffer* dstBuffer = ResourceManager::ptr->getBuffer(dst);
		FAILIF(!dstBuffer);

		copyBufferToBuffer(srcBuffer, dstBuffer, numBytes, srcOffset, dstOffset);
	}

	void Device::copyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		transitionResource(src, D3D12_RESOURCE_STATE_GENERIC_READ);
		transitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST);
		submitResourceBarriers();

		m_commandList->CopyBufferRegion(dst->resource.Get(), dstOffset, src->resource.Get(), srcOffset, numBytes);
	}

	void Device::bindVertexBuffer(Handle<Buffer> buffer)
	{
		ResourceManager* rm = ResourceManager::ptr;
		Buffer* vb = rm->getBuffer(buffer);
		transitionResource(vb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = vb->resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)vb->byteSize,
			.StrideInBytes = vb->byteStride
		};
		m_commandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void Device::bindIndexBuffer(Handle<Buffer> buffer)
	{
		ResourceManager* rm = ResourceManager::ptr;
		Buffer* ib = rm->getBuffer(buffer);
		transitionResource(ib, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		D3D12_INDEX_BUFFER_VIEW ibView = {
			.BufferLocation = ib->resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)ib->byteSize,
			.Format = DXGI_FORMAT_R32_UINT
		};

		m_commandList->IASetIndexBuffer(&ibView);
	}

	void Device::bindShader(Handle<Shader> shader)
	{
		m_boundShader = shader;

		ResourceManager* rm = ResourceManager::ptr;
		Shader* pBoundShader = rm->getShader(m_boundShader);

		if (pBoundShader->type == ShaderType::Graphics)
		{
			int32 width  = m_swapchain->getBackbufferWidth();
			int32 height = m_swapchain->getBackbufferHeight();
			if (pBoundShader->useSwapchainRT)
			{
				bindSwapchainRenderTargets();
			}
			else
			{
				// first transition the render targets to the correct resource state
				for (uint8 i = 0; i < pBoundShader->rtCount; ++i)
					transitionResource(pBoundShader->renderTargets[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
				submitResourceBarriers();

				Texture* depthBackbuffer = rm->getTexture(pBoundShader->depthTarget);
				FAILIF(!depthBackbuffer);

				constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				m_commandList->ClearDepthStencilView(depthBackbuffer->handle.cpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

				D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[8];
				for (uint8 i = 0; i < pBoundShader->rtCount; ++i)
				{
					Texture* rt = rm->getTexture(pBoundShader->renderTargets[i]);
					FAILIF(!rt);

					m_commandList->ClearRenderTargetView(rt->handle.cpuHandle, clearColor, 0, nullptr);
					rtHandles[i] = rt->handle.cpuHandle;
				}

				m_commandList->OMSetRenderTargets(pBoundShader->rtCount, rtHandles, false, &depthBackbuffer->handle.cpuHandle);
			}

			D3D12_VIEWPORT viewport = {
				.TopLeftX = 0,
				.TopLeftY = 0,
				.Width = (float)width,
				.Height = (float)height,
				.MinDepth = 0.0f,
				.MaxDepth = 1.0f
			};

			D3D12_RECT scissor = {
				.left = 0,
				.top = 0,
				.right = width,
				.bottom = height
			};

			m_commandList->RSSetViewports(1, &viewport);
			m_commandList->RSSetScissorRects(1, &scissor);
		}
	}

	void Device::installDrawState()
	{
		submitResourceBarriers();

		ResourceManager* rm = ResourceManager::ptr;
		Shader* shader = rm->getShader(m_boundShader);

		if (shader->type == ShaderType::Compute)
		{
			m_commandList->SetComputeRootSignature(shader->rootSignature.Get());
			shader->setComputeRootParameters(m_commandList.Get());
		}
		else if (shader->type == ShaderType::Graphics)
		{
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_commandList->SetGraphicsRootSignature(shader->rootSignature.Get());
			shader->setGraphicsRootParameters(m_commandList.Get());
		}

		m_commandList->SetPipelineState(shader->pipelineState.Get());
	}

	void Device::bindSwapchainRenderTargets()
	{
		ResourceManager* rm = ResourceManager::ptr;

		Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
		Texture* backbuffer = rm->getTexture(backBufferHandle);
		FAILIF(!backbuffer);

		Handle<Texture> depthBackBufferHandle = m_swapchain->getDepthBackbuffer(m_frameIndex);
		Texture* depthBackbuffer = rm->getTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);

		m_commandList->OMSetRenderTargets(1, &backbuffer->handle.cpuHandle, false, &depthBackbuffer->handle.cpuHandle);

		constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_commandList->ClearDepthStencilView(depthBackbuffer->handle.cpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_commandList->ClearRenderTargetView(backbuffer->handle.cpuHandle, clearColor, 0, nullptr);
	}

	void Device::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		installDrawState();
		m_commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void Device::drawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance)
	{
		installDrawState();
		m_commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	void Device::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		installDrawState();
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
		if (m_flags & GfxDeviceFlag::EnableImgui)
		{
			beginEvent("ImGui");

			ResourceManager* rm = ResourceManager::ptr;
			Shader* shader = rm->getShader(m_boundShader);
			if (!shader->useSwapchainRT)
				bindSwapchainRenderTargets();

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

			endEvent();
		}

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

		DX_CHECK(m_commandAllocators[m_frameIndex]->Reset());
		DX_CHECK(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

		ID3D12DescriptorHeap* heaps[] = { m_srvheap->getHeap(), m_samplerheap->getHeap() };
		m_commandList->SetDescriptorHeaps(2, heaps);

		// Prepare frame render targets
		{
			Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
			Texture* backbuffer = ResourceManager::ptr->getTexture(backBufferHandle);
			FAILIF(!backbuffer);
			transitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

			Handle<Texture> depthBackBufferHandle = m_swapchain->getDepthBackbuffer(m_frameIndex);
			Texture* depthBackbuffer = ResourceManager::ptr->getTexture(depthBackBufferHandle);
			FAILIF(!depthBackbuffer);
			transitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			submitResourceBarriers();
		}

		if (m_flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	Format Device::getSwapchainFormat()
	{
		return m_swapchain->getFormat();
	}

	Format Device::getSwapchainDepthFormat()
	{
		return m_swapchain->getDepthFormat();
	}

	void Device::initSwapchainBackBuffers()
	{
		m_swapchain->initBackBuffers();

		Handle<Texture> backBufferHandle = m_swapchain->getBackbuffer(m_frameIndex);
		Texture* backbuffer = ResourceManager::ptr->getTexture(backBufferHandle);
		FAILIF(!backbuffer);
		transitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		Handle<Texture> depthBackBufferHandle = m_swapchain->getDepthBackbuffer(m_frameIndex);
		Texture* depthBackbuffer = ResourceManager::ptr->getTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);
		transitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

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
		case DescriptorHeapType::SAMPLERS:
			return m_samplerheap->allocateHandle();
		default: 
			return DescriptorHandle();
		}
	}

	void Device::transitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState)
	{
		Texture* t = ResourceManager::ptr->getTexture(texture);
		FAILIF(!t);
		transitionResource(t, newState);
	}

	void Device::transitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState)
	{
		Buffer* b = ResourceManager::ptr->getBuffer(buffer);
		FAILIF(!b);
		transitionResource(b, newState);
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

	void Device::beginEvent(const char* name, uint64 color)
	{
		PIXBeginEvent(m_commandList.Get(), color, name);
	}

	void Device::endEvent()
	{
		PIXEndEvent(m_commandList.Get());
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

		utils::StringConvert(desc.Description, m_gpuInfo.name);
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

	void Device::onWindowClose()
	{
		waitGPU();
	}
}

#include "stdafx.h"
#include "device.h"

#include "core/window.h"
#include "gfx/gfx.h"
#include "swapchain.h"
#include "descriptorheap.h"

#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_glfw.h>

unsigned int DelegateHandle::CURRENT_ID = 0;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo::Gfx
{
	Device::Device(Core::Window* window, GfxDeviceFlags flags)
		: m_Flags(flags), m_GPUInfo()
	{
		uint32_t dxgiFactoryFlags = 0;

		HMODULE pixGPUCapturer = PIXLoadLatestWinPixGpuCapturerLibrary();
		if (pixGPUCapturer)
		{
			DX_CHECK(PIXSetTargetWindow(window->GetWin32Handle()));
			DX_CHECK(PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS));
			m_bPIXCanCapture = true;
		}

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

		DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)));

		PickGPU();

		DX_CHECK(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_Device)));

#if !NO_LOG
		// RenderDoc does not support ID3D12InfoQueue1 so do not enable it when running under it
		{
			bool bUnderRenderDoc = false;

			IID renderDocID;
			if (SUCCEEDED(IIDFromString(L"{A7AA6116-9C8D-4BBA-9083-B4D816B71B78}", &renderDocID)))
			{
				ComPtr<IUnknown> renderDoc;
				if (SUCCEEDED(m_Device->QueryInterface(renderDocID, &renderDoc)))
				{
					// Running under RenderDoc, so enable capturing mode
					bUnderRenderDoc = true;
					LB_LOG("Running under Render Doc, ID3D12InfoQueue features won't be enabled!");
				}
			}

			if (!bUnderRenderDoc)
			{
				ComPtr<ID3D12InfoQueue> d3d12InfoQueue;
				DX_CHECK(m_Device->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue)));
				ComPtr<ID3D12InfoQueue1> d3d12InfoQueue1;
				DX_CHECK(d3d12InfoQueue->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue1)));
				DWORD messageCallbackCookie;
				DX_CHECK(d3d12InfoQueue1->RegisterMessageCallback(Internal::DXMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &messageCallbackCookie));
			}
		}
#endif

		m_Srvheap = new DescriptorHeap(m_Device.Get(), DescriptorHeapType::SRV, true);
		m_Rtvheap = new DescriptorHeap(m_Device.Get(), DescriptorHeapType::RTV);
		m_Dsvheap = new DescriptorHeap(m_Device.Get(), DescriptorHeapType::DSV);
		m_Samplerheap = new DescriptorHeap(m_Device.Get(), DescriptorHeapType::SAMPLERS, true);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

		m_Swapchain = new Swapchain(m_CommandQueue.Get(), m_Factory.Get(), window);
		OnPostResourceManagerInit.AddRaw(this, &Device::InitSwapchainBackBuffers);
		OnPreResourceManagerShutdown.AddRaw(this, &Device::DestroySwapchainBackBuffers);
		OnPreResourceManagerShutdown.AddRaw(this, &Device::WaitGPU);

		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		for (uint8 i = 0; i < NUM_BACK_BUFFERS; ++i)
			DX_CHECK(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i])));

		DX_CHECK(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[m_FrameIndex].Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));

		// Create synchronization objects
		{
			DX_CHECK(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
			m_FenceValues[m_FrameIndex]++;

			// Create an event handler to use for frame synchronization
			m_FenceEvent = CreateEvent(nullptr, false, false, nullptr);
			ensure(m_FenceEvent);
		}

		ID3D12DescriptorHeap* heaps[] = { m_Srvheap->GetHeap(), m_Samplerheap->GetHeap() };
		m_CommandList->SetDescriptorHeaps(2, heaps);

		// ImGui Stuff
		if (flags & GfxDeviceFlag::EnableImgui)
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans/NotoSans-Regular.ttf", 18.0f);
			ImGuiStyle& style = ImGui::GetStyle();
			style.ItemInnerSpacing = ImVec2(10.0f, 0.0f);

			DescriptorHandle imguiDescriptor = m_Srvheap->AllocateHandle();

			ImGui_ImplGlfw_InitForOther(window->GetGlfwHandle(), true);
			ImGui_ImplDX12_Init(m_Device.Get(), NUM_BACK_BUFFERS, D3DFormat(m_Swapchain->GetFormat()),
			                    m_Srvheap->GetHeap(), imguiDescriptor.CpuHandle, imguiDescriptor.GPUHandle);

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	Device::~Device()
	{
		WaitGPU();

		delete m_Srvheap;
		delete m_Rtvheap;
		delete m_Dsvheap;
		delete m_Samplerheap;

#if !NO_LOG
		if (m_Flags & GfxDeviceFlag::DetailedLogging)
		{
			IDXGIDebug1* dxgiDebug;
			DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
			DX_CHECK(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
			dxgiDebug->Release();
		}
#endif

		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
	}

	void Device::DestroySwapchainBackBuffers()
	{
		delete m_Swapchain;
	}

	void Device::CopyTextureToBackBuffer(Handle<Texture> texture)
	{
		ResourceManager* rm = ResourceManager::Ptr;

		Texture* d3dTexture = rm->GetTexture(texture);

		Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
		Texture* backbuffer = rm->GetTexture(backBufferHandle);

		TransitionResource(d3dTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
		TransitionResource(backbuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		SubmitResourceBarriers();
		m_CommandList->CopyResource(backbuffer->Resource.Get(), d3dTexture->Resource.Get());

		TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		SubmitResourceBarriers();
	}

	void Device::CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst)
	{
		Texture* dstTexture = ResourceManager::Ptr->GetTexture(dst);
		FAILIF(!dstTexture);
		Buffer* srcBuffer = ResourceManager::Ptr->GetBuffer(src);
		FAILIF(!srcBuffer);

		CopyBufferToTexture(srcBuffer, dstTexture);
	}

	void Device::CopyBufferToTexture(Buffer* src, Texture* dst)
	{
		D3D12_RESOURCE_DESC dstDesc = dst->Resource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprints[D3D12_REQ_MIP_LEVELS] = {};
		m_Device->GetCopyableFootprints(&dstDesc, 0, dstDesc.MipLevels, 0, srcFootprints, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {
			.pResource = src->Resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = srcFootprints[0]
		};

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {
			.pResource = dst->Resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = 0
		};
		m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	void Device::CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		Buffer* srcBuffer = ResourceManager::Ptr->GetBuffer(src);
		FAILIF(!srcBuffer);
		Buffer* dstBuffer = ResourceManager::Ptr->GetBuffer(dst);
		FAILIF(!dstBuffer);

		CopyBufferToBuffer(srcBuffer, dstBuffer, numBytes, srcOffset, dstOffset);
	}

	void Device::CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		TransitionResource(src, D3D12_RESOURCE_STATE_GENERIC_READ);
		TransitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST);
		SubmitResourceBarriers();

		m_CommandList->CopyBufferRegion(dst->Resource.Get(), dstOffset, src->Resource.Get(), srcOffset, numBytes);
	}

	void Device::BindVertexBuffer(Handle<Buffer> buffer)
	{
		ResourceManager* rm = ResourceManager::Ptr;
		Buffer* vb = rm->GetBuffer(buffer);
		TransitionResource(vb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = vb->Resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)vb->ByteSize,
			.StrideInBytes = vb->ByteStride
		};
		m_CommandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void Device::BindIndexBuffer(Handle<Buffer> buffer)
	{
		ResourceManager* rm = ResourceManager::Ptr;
		Buffer* ib = rm->GetBuffer(buffer);
		TransitionResource(ib, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		D3D12_INDEX_BUFFER_VIEW ibView = {
			.BufferLocation = ib->Resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)ib->ByteSize,
			.Format = DXGI_FORMAT_R32_UINT
		};

		m_CommandList->IASetIndexBuffer(&ibView);
	}

	void Device::BindShader(Handle<Shader> shader)
	{
		m_BoundShader = shader;

		ResourceManager* rm = ResourceManager::Ptr;
		Shader* pBoundShader = rm->GetShader(m_BoundShader);

		if (pBoundShader->Type == ShaderType::Graphics)
		{
			int32 width  = m_Swapchain->GetBackbufferWidth();
			int32 height = m_Swapchain->GetBackbufferHeight();
			if (pBoundShader->UseSwapchainRT)
			{
				BindSwapchainRenderTargets();
			}
			else
			{
				// first transition the render targets to the correct resource state
				for (uint8 i = 0; i < pBoundShader->RTCount; ++i)
					TransitionResource(pBoundShader->RenderTargets[i].Texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
				SubmitResourceBarriers();

				D3D12_CPU_DESCRIPTOR_HANDLE* dsvhandle = nullptr;

				if (pBoundShader->DepthTarget.Texture.IsValid())
				{
					Texture* depthBackbuffer = rm->GetTexture(pBoundShader->DepthTarget.Texture);
					FAILIF(!depthBackbuffer);
					dsvhandle = &depthBackbuffer->BasicHandle.CpuHandle;
					if (pBoundShader->DepthTarget.LoadRenderPassOp == RenderPassOp::Clear)
						m_CommandList->ClearDepthStencilView(depthBackbuffer->BasicHandle.CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
				}
				
				constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[8];
				for (uint8 i = 0; i < pBoundShader->RTCount; ++i)
				{
					Texture* rt = rm->GetTexture(pBoundShader->RenderTargets[i].Texture);
					FAILIF(!rt);

					if (pBoundShader->RenderTargets[i].LoadRenderPassOp == RenderPassOp::Clear)
						m_CommandList->ClearRenderTargetView(rt->BasicHandle.CpuHandle, clearColor, 0, nullptr);

					rtHandles[i] = rt->BasicHandle.CpuHandle;
				}

				m_CommandList->OMSetRenderTargets(pBoundShader->RTCount, rtHandles, false, dsvhandle);
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

			m_CommandList->RSSetViewports(1, &viewport);
			m_CommandList->RSSetScissorRects(1, &scissor);
		}
	}

	void Device::InstallDrawState()
	{
		SubmitResourceBarriers();

		ResourceManager* rm = ResourceManager::Ptr;
		Shader* shader = rm->GetShader(m_BoundShader);

		if (shader->Type == ShaderType::Compute)
		{
			m_CommandList->SetComputeRootSignature(shader->RootSignature.Get());
			shader->SetComputeRootParameters(m_CommandList.Get());
		}
		else if (shader->Type == ShaderType::Graphics)
		{
			m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_CommandList->SetGraphicsRootSignature(shader->RootSignature.Get());
			shader->SetGraphicsRootParameters(m_CommandList.Get());
		}

		m_CommandList->SetPipelineState(shader->PipelineState.Get());
	}

	void Device::BindSwapchainRenderTargets()
	{
		ResourceManager* rm = ResourceManager::Ptr;

		Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
		Texture* backbuffer = rm->GetTexture(backBufferHandle);
		FAILIF(!backbuffer);

		Handle<Texture> depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
		Texture* depthBackbuffer = rm->GetTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);

		m_CommandList->OMSetRenderTargets(1, &backbuffer->BasicHandle.CpuHandle, false, &depthBackbuffer->BasicHandle.CpuHandle);

		constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_CommandList->ClearDepthStencilView(depthBackbuffer->BasicHandle.CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_CommandList->ClearRenderTargetView(backbuffer->BasicHandle.CpuHandle, clearColor, 0, nullptr);
	}

	void Device::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		InstallDrawState();
		m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void Device::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance)
	{
		InstallDrawState();
		m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	void Device::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		InstallDrawState();
		m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void Device::SubmitResourceBarriers()
	{
		if (m_ResourceBarriers.empty()) return;
		m_CommandList->ResourceBarrier((uint32)m_ResourceBarriers.size(), m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}

	void Device::Present()
	{
		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			BeginEvent("ImGui");

			ResourceManager* rm = ResourceManager::Ptr;
			Shader* shader = rm->GetShader(m_BoundShader);
			if (!shader->UseSwapchainRT)
				BindSwapchainRenderTargets();

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

			EndEvent();
		}

		{
			Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
			TransitionResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT);

			SubmitResourceBarriers();
		}

		DX_CHECK(m_CommandList->Close());

		ID3D12CommandList* cmd[1] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(1, cmd);
		m_Swapchain->Present(!(m_Flags & GfxDeviceFlag::DisableVSync));

		NextFrame();

		ResourceManager::Ptr->RunDeletionQueue();

		DX_CHECK(m_CommandAllocators[m_FrameIndex]->Reset());
		DX_CHECK(m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr));

		ID3D12DescriptorHeap* heaps[] = { m_Srvheap->GetHeap(), m_Samplerheap->GetHeap() };
		m_CommandList->SetDescriptorHeaps(2, heaps);

		// Prepare frame render targets
		{
			Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
			FAILIF(!backbuffer);
			TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

			Handle<Texture> depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
			Texture* depthBackbuffer = ResourceManager::Ptr->GetTexture(depthBackBufferHandle);
			FAILIF(!depthBackbuffer);
			TransitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			SubmitResourceBarriers();
		}

		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	Format Device::GetSwapchainFormat()
	{
		return m_Swapchain->GetFormat();
	}

	Format Device::GetSwapchainDepthFormat()
	{
		return m_Swapchain->GetDepthFormat();
	}

	void Device::InitSwapchainBackBuffers()
	{
		m_Swapchain->InitBackBuffers();

		Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
		Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
		FAILIF(!backbuffer);
		TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		Handle<Texture> depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
		Texture* depthBackbuffer = ResourceManager::Ptr->GetTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);
		TransitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		SubmitResourceBarriers();
	}

	DescriptorHandle Device::AllocateHandle(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV:
			return m_Srvheap->AllocateHandle();
		case DescriptorHeapType::RTV:
			return m_Rtvheap->AllocateHandle();
		case DescriptorHeapType::DSV:
			return m_Dsvheap->AllocateHandle();
		case DescriptorHeapType::SAMPLERS:
			return m_Samplerheap->AllocateHandle();
		default: 
			return DescriptorHandle();
		}
	}

	void Device::TransitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState)
	{
		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t);
		TransitionResource(t, newState);
	}

	void Device::TransitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState)
	{
		Buffer* b = ResourceManager::Ptr->GetBuffer(buffer);
		FAILIF(!b);
		TransitionResource(b, newState);
	}

	void Device::TransitionResource(Texture* texture, D3D12_RESOURCE_STATES newState)
	{
		if (texture->CurrentState == newState) return;
		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(), texture->CurrentState, newState));
		texture->CurrentState = newState;
	}

	void Device::TransitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState)
	{
		if (buffer->CurrentState == newState) return;
		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(buffer->Resource.Get(), buffer->CurrentState, newState));
		buffer->CurrentState = newState;
	}

	void Device::BeginEvent(const char* name, uint64 color)
	{
		PIXBeginEvent(m_CommandList.Get(), color, name);
	}

	void Device::EndEvent()
	{
		PIXEndEvent(m_CommandList.Get());
	}

	void Device::TakeGPUCapture()
	{
		wchar_t currDir[MAX_PATH];
		GetCurrentDirectoryW(MAX_PATH, currDir);
		std::wstring directory = currDir;
		directory.append(L"\\gpucaptures\\");
		if (!Utils::PathExists(directory.c_str()))
			CreateDirectoryW(directory.c_str(), 0);
		std::wstring filename = directory;
		filename.append(L"LB_");
		filename.append(std::format(L"{:%d_%m_%Y_%H_%M_%OS}", std::chrono::system_clock::now()));
		filename.append(L".wpix");

		DX_CHECK(PIXGpuCaptureNextFrames(filename.c_str(), 1));
		LB_WLOG("Saved the GPU capture to '%ls'", filename.c_str());
	}

	uint32 Device::GetBackbufferWidth()
	{
		return m_Swapchain->GetBackbufferWidth();
	}

	uint32 Device::GetBackbufferHeight()
	{
		return m_Swapchain->GetBackbufferHeight();
	}

	void Device::PickGPU()
	{
		m_Adapter = nullptr;

		// Get IDXGIFactory6
		ComPtr<IDXGIFactory6> factory6;
		DX_CHECK(m_Factory->QueryInterface(IID_PPV_ARGS(&factory6)));
		for (uint32_t adapterIndex = 0;
			!FAILED(factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_Adapter)));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			m_Adapter->GetDesc1(&desc);

			// Don't select the Basic Render Driver adapter.
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
				break;
		}

		if (!ensure(m_Adapter))
			LB_ERROR("Failed to pick an adapter");

		DXGI_ADAPTER_DESC1 desc;
		DX_CHECK(m_Adapter->GetDesc1(&desc));
		LB_WLOG("Initiliazed D3D12 on %ls", desc.Description);

		Utils::StringConvert(desc.Description, m_GPUInfo.Name);
	}

	void Device::NextFrame()
	{
		uint64 currentFenceValue = m_FenceValues[m_FrameIndex];
		DX_CHECK(m_CommandQueue->Signal(m_Fence.Get(), currentFenceValue));

		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		if (m_Fence->GetCompletedValue() < m_FenceValues[m_FrameIndex])
		{
			DX_CHECK(m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent));
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}

		m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
	}

	void Device::WaitGPU()
	{
		// Schedule a signal command in the queue
		m_CommandQueue->Signal(m_Fence.Get(), m_FenceValues[m_FrameIndex]);
		
		// Wait until the fence has been processed
		m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
		WaitForSingleObject(m_FenceEvent, INFINITE);
		
		// Increment the fence value for the current frame
		m_FenceValues[m_FrameIndex]++;
	}
}

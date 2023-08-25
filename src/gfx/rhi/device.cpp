#include "stdafx.h"
#include "device.h"
#include "commandcontext.h"
#include "commandqueue.h"
#include "swapchain.h"
#include "descriptorheap.h"
#include "ringbufferallocator.h"
#include "core/window.h"
#include "gfx/gfx.h"

#include <comdef.h>

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
		bool bIsProfiling = Core::CommandLine::HasArg(LIMBO_CMD_PROFILE);

#if !NO_LOG
		if (!bIsProfiling)
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)));

#if !NO_LOG
		if (!bIsProfiling)
		{
			// note: disabled pix capture for now, it redirects the GPU validation into their stuff, so the app does not get any GPU validation errors
			if (!IsUnderPIX() && false) 
			{
				HMODULE pixGPUCapturer = PIXLoadLatestWinPixGpuCapturerLibrary();
				if (pixGPUCapturer)
				{
					DX_CHECK(PIXSetTargetWindow(window->GetWin32Handle()));
					DX_CHECK(PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS));
					m_bPIXCanCapture = true;
					LB_LOG("PIX capture enabled.");
				}
			}

			ComPtr<ID3D12Debug> debugController;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();

#if LIMBO_DEBUG
			ComPtr<ID3D12Debug1> debugController1;
			DX_CHECK(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)));
			debugController1->SetEnableGPUBasedValidation(true);
#endif
		}
#endif

		PickGPU();

		constexpr D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		DX_CHECK(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_Device.GetAddressOf())));
		D3D12_FEATURE_DATA_FEATURE_LEVELS caps{};
		caps.pFeatureLevelsRequested = featureLevels;
		caps.NumFeatureLevels = ARRAYSIZE(featureLevels);
		DX_CHECK(m_Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &caps, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS)));
		DX_CHECK(D3D12CreateDevice(m_Adapter.Get(), caps.MaxSupportedFeatureLevel, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())));

		DX_CHECK(m_FeatureSupport.Init(m_Device.Get()));
		check(m_FeatureSupport.ResourceBindingTier() >= D3D12_RESOURCE_BINDING_TIER_3);
		check(m_FeatureSupport.HighestShaderModel() >= D3D_SHADER_MODEL_6_6);
		check(m_FeatureSupport.RaytracingTier() == D3D12_RAYTRACING_TIER_1_1);

#if !NO_LOG
		// RenderDoc does not support ID3D12InfoQueue1 so do not enable it when running under it
		if (!bIsProfiling && !IsUnderRenderDoc())
		{
			ComPtr<ID3D12InfoQueue> d3d12InfoQueue;
			DX_CHECK(m_Device->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue)));
			ComPtr<ID3D12InfoQueue1> d3d12InfoQueue1;
			if (SUCCEEDED(d3d12InfoQueue->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue1))))
			{
				// Suppress messages based on their severity level
				D3D12_MESSAGE_SEVERITY Severities[] =
				{
					D3D12_MESSAGE_SEVERITY_INFO
				};

				// Suppress individual messages by their ID
				D3D12_MESSAGE_ID DenyIds[] =
				{
					// This occurs when there are uninitialized descriptors in a descriptor table, even when a
					// shader does not access the missing descriptors.
					D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
				};

				D3D12_INFO_QUEUE_FILTER NewFilter = {};
				NewFilter.DenyList.NumSeverities = _countof(Severities);
				NewFilter.DenyList.pSeverityList = Severities;
				NewFilter.DenyList.NumIDs = _countof(DenyIds);
				NewFilter.DenyList.pIDList = DenyIds;

				d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

				d3d12InfoQueue->PushStorageFilter(&NewFilter);

				DWORD messageCallbackCookie;
				DX_CHECK(d3d12InfoQueue1->RegisterMessageCallback(Internal::DXMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &messageCallbackCookie));
			}
			else
			{
				LB_WARN("Failed to get ID3D12InfoQueue1, there will be no message callback.");
			}
		}
#endif

		m_GlobalHeap	= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::SRV,    8196, true);
		m_Rtvheap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::RTV,     128);
		m_Dsvheap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::DSV,      64);

		for (int i = 0; i < (int)ContextType::MAX; ++i)
		{
			m_CommandQueues[i] = new CommandQueue((ContextType)i, this);
			m_CommandContexts[i] = new CommandContext(m_CommandQueues[i], (ContextType)i, m_Device.Get(), m_GlobalHeap);
		}

		m_Swapchain = new Swapchain(m_CommandQueues[(int)ContextType::Direct]->Get(), m_Factory.Get(), window);
		OnPostResourceManagerInit.AddRaw(this, &Device::InitResources);
		OnPreResourceManagerShutdown.AddRaw(this, &Device::DestroyResources);
		OnPreResourceManagerShutdown.AddRaw(this, &Device::IdleGPU);

		window->OnWindowResize.AddRaw(this, &Device::HandleWindowResize);

		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		m_PresentFence = new Fence(this);

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

			DescriptorHandle imguiDescriptor = m_GlobalHeap->AllocateHandle();

			ImGui_ImplGlfw_InitForOther(window->GetGlfwHandle(), true);
			ImGui_ImplDX12_Init(m_Device.Get(), NUM_BACK_BUFFERS, D3DFormat(m_Swapchain->GetFormat()),
			                    m_GlobalHeap->GetHeap(), imguiDescriptor.CpuHandle, imguiDescriptor.GPUHandle);

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	Device::~Device()
	{
		IdleGPU();

		delete m_PresentFence;

		delete m_GlobalHeap;
		delete m_Rtvheap;
		delete m_Dsvheap;

		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		for (int i = 0; i < (int)ContextType::MAX; ++i)
		{
			delete m_CommandContexts.at(i);
			delete m_CommandQueues.at(i);
		}
		
#if !NO_LOG
		if (m_Flags & GfxDeviceFlag::DetailedLogging)
		{
			IDXGIDebug1* dxgiDebug;
			DX_CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));
			DX_CHECK(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
			dxgiDebug->Release();
		}
#endif
	}

	void Device::DestroyResources()
	{
		DestroyShader(m_GenerateMipsShader);

		delete m_UploadRingBuffer;
		delete m_Swapchain;
	}

	void Device::Present(bool bEnableVSync)
	{
		CommandContext* context = m_CommandContexts.at((int)ContextType::Direct);

		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			BeginEvent("ImGui");

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context->Get());

			EndEvent();
		}

		{
			Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
			context->TransitionResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT);

			context->SubmitResourceBarriers();
		}

		context->Execute();
		m_Swapchain->Present(bEnableVSync);
		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		uint64 presentValue = m_PresentFence->Signal(m_CommandQueues[(int)ContextType::Direct]);
		m_PresentFence->CpuWait(presentValue);

		ResourceManager::Ptr->RunDeletionQueue();

		if (m_bNeedsResize)
		{
			IdleGPU();
			m_Swapchain->CheckForResize();
			m_bNeedsResize = false;
			m_FrameIndex = m_Swapchain->GetCurrentIndex();
		}

		if (m_bNeedsShaderReload)
		{
			Core::Timer t;
			IdleGPU();
			ReloadShaders.Broadcast();
			m_bNeedsShaderReload = false;
			LB_LOG("Shaders got reloaded. It took %.2fs", t.ElapsedSeconds());
		}

		// Prepare frame render targets
		{
			Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
			FAILIF(!backbuffer);
			context->TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

			Handle<Texture> depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
			Texture* depthBackbuffer = ResourceManager::Ptr->GetTexture(depthBackBufferHandle);
			FAILIF(!depthBackbuffer);
			context->TransitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

			context->SubmitResourceBarriers();
		}

		if (m_Flags & GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	void Device::HandleWindowResize(uint32 width, uint32 height)
	{
		m_bNeedsResize = true;
		m_Swapchain->MarkForResize(width, height);
	}

	Handle<Texture> Device::GetCurrentBackbuffer() const
	{
		return m_Swapchain->GetBackbuffer(m_FrameIndex);
	}

	Handle<Texture> Device::GetCurrentDepthBackbuffer() const
	{
		return m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
	}

	Format Device::GetSwapchainFormat()
	{
		return m_Swapchain->GetFormat();
	}

	Format Device::GetSwapchainDepthFormat()
	{
		return m_Swapchain->GetDepthFormat();
	}

	bool Device::CanTakeGPUCapture()
	{
		return m_bPIXCanCapture;
	}

	void Device::InitResources()
	{
		m_Swapchain->InitBackBuffers();

		Handle<Texture> backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
		Texture* backbuffer = ResourceManager::Ptr->GetTexture(backBufferHandle);
		FAILIF(!backbuffer);
		GetCommandContext(ContextType::Direct)->TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);

		Handle<Texture> depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
		Texture* depthBackbuffer = ResourceManager::Ptr->GetTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);
		GetCommandContext(ContextType::Direct)->TransitionResource(depthBackbuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		GetCommandContext(ContextType::Direct)->SubmitResourceBarriers();

		m_GenerateMipsShader = CreateShader({
			.ProgramName = "generatemips",
			.CsEntryPoint = "GenerateMip",
			.Type = ShaderType::Compute
		});

		m_UploadRingBuffer = new RingBufferAllocator(Utils::ToMB(256));
	}

	DescriptorHandle Device::AllocateHandle(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV:
			return m_GlobalHeap->AllocateHandle();
		case DescriptorHeapType::RTV:
			return m_Rtvheap->AllocateHandle();
		case DescriptorHeapType::DSV:
			return m_Dsvheap->AllocateHandle();
		default: 
			return DescriptorHandle();
		}
	}

	void Device::TakeGPUCapture()
	{
		if (!m_bPIXCanCapture) return;

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

		m_LastGPUCaptureFilename = std::move(filename);
	}

	void Device::OpenLastGPUCapture()
	{
		if (m_LastGPUCaptureFilename.empty()) return;

		LB_LOG("Launching PIX...");
		HINSTANCE ret = PIXOpenCaptureInUI(m_LastGPUCaptureFilename.c_str());
		if ((uintptr_t)ret <= 32)
		{
			_com_error err(HRESULT_FROM_WIN32(GetLastError()));
			const char* errMsg = err.ErrorMessage();
			LB_LOG("Failed to launch PIX: %s", errMsg);
		}
	}

	uint32 Device::GetBackbufferWidth()
	{
		return m_Swapchain->GetBackbufferWidth();
	}

	uint32 Device::GetBackbufferHeight()
	{
		return m_Swapchain->GetBackbufferHeight();
	}

	void Device::MarkReloadShaders()
	{
		m_bNeedsShaderReload = true;
	}

	void Device::GenerateMipLevels(Handle<Texture> texture)
	{
		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t);

		BindShader(m_GenerateMipsShader);
		for (uint16 i = 1; i < t->Spec.MipLevels; ++i)
		{
			uint2 outputMipSize = { t->Spec.Width , t->Spec.Height };
			outputMipSize.x >>= i;
			outputMipSize.y >>= i;
			float2 texelSize = { 1.0f / outputMipSize.x, 1.0f / outputMipSize.y };

			SetParameter(m_GenerateMipsShader, "TexelSize", texelSize);
			SetParameter(m_GenerateMipsShader, "PreviousMip", texture, i - 1);
			SetParameter(m_GenerateMipsShader, "OutputMip", texture, i);
			Dispatch(Math::Max(outputMipSize.x / 8, 1u), Math::Max(outputMipSize.y / 8, 1u), 1);
			GetCommandContext(ContextType::Direct)->UAVBarrier(texture);
		}

		GetCommandContext(ContextType::Direct)->TransitionResource(texture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, t->Spec.MipLevels - 1);
		GetCommandContext(ContextType::Direct)->SubmitResourceBarriers();
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

	void Device::IdleGPU()
	{
		for (int i = 0; i < (int)ContextType::MAX; ++i)
			m_CommandQueues[i]->WaitForIdle();
	}

	bool Device::IsUnderPIX()
	{
		IID GraphicsAnalysisID;
		if (SUCCEEDED(IIDFromString(L"{9F251514-9D4D-4902-9D60-18988AB7D4B5}", &GraphicsAnalysisID)))
		{
			ComPtr<IUnknown> GraphicsAnalysis;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, GraphicsAnalysisID, (void**)GraphicsAnalysis.ReleaseAndGetAddressOf())))
			{
				// Running under PIX
				return true;
			}
		}
		return false;
	}

	bool Device::IsUnderRenderDoc()
	{
		IID renderDocID;
		if (SUCCEEDED(IIDFromString(L"{A7AA6116-9C8D-4BBA-9083-B4D816B71B78}", &renderDocID)))
		{
			ComPtr<IUnknown> renderDoc;
			if (SUCCEEDED(m_Device->QueryInterface(renderDocID, &renderDoc)))
			{
				// Running under RenderDoc, so enable capturing mode
				LB_LOG("Running under Render Doc, ID3D12InfoQueue features won't be enabled!");
				return true;
			}
		}

		return false;
	}
}

#include "stdafx.h"
#include "device.h"
#include "commandcontext.h"
#include "commandqueue.h"
#include "swapchain.h"
#include "descriptorheap.h"
#include "ringbufferallocator.h"
#include "shadercompiler.h"
#include "core/window.h"
#include "core/utils.h"
#include "core/timer.h"
#include "gfx/gfx.h"
#include "pipelinestateobject.h"

#include <comdef.h> // _com_error
#include <dxgidebug.h>

#include <imgui/backends/imgui_impl_dx12.h>
#include <imgui/backends/imgui_impl_glfw.h>

unsigned int DelegateHandle::CURRENT_ID = 0;

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo::RHI
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
		DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_Factory.ReleaseAndGetAddressOf())));

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

			RefCountPtr<ID3D12Debug> debugController;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf())));
			debugController->EnableDebugLayer();

#if LIMBO_DEBUG
			RefCountPtr<ID3D12Debug1> debugController1;
			DX_CHECK(debugController->QueryInterface(IID_PPV_ARGS(debugController1.ReleaseAndGetAddressOf())));
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
			RefCountPtr<ID3D12InfoQueue> d3d12InfoQueue;
			DX_CHECK(m_Device->QueryInterface(IID_PPV_ARGS(d3d12InfoQueue.ReleaseAndGetAddressOf())));
			RefCountPtr<ID3D12InfoQueue1> d3d12InfoQueue1;
			if (SUCCEEDED(d3d12InfoQueue->QueryInterface(IID_PPV_ARGS(d3d12InfoQueue1.ReleaseAndGetAddressOf()))))
			{
				// Suppress messages based on their severity level
				D3D12_MESSAGE_SEVERITY Severities[] =
				{
					D3D12_MESSAGE_SEVERITY_INFO
				};

				D3D12_INFO_QUEUE_FILTER NewFilter = {};
				NewFilter.DenyList.NumSeverities = _countof(Severities);
				NewFilter.DenyList.pSeverityList = Severities;

				DX_CHECK(d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true));
				DX_CHECK(d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true));
				DX_CHECK(d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false));

				DX_CHECK(d3d12InfoQueue->PushStorageFilter(&NewFilter));

				DWORD messageCallbackCookie;
				DX_CHECK(d3d12InfoQueue1->RegisterMessageCallback(Internal::DXMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &messageCallbackCookie));
			}
			else
			{
				LB_WARN("Failed to get ID3D12InfoQueue1, there will be no message callback.");
			}
		}
#endif

		m_GlobalHeap	= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::SRV, 4098, 4098, true);
		m_UAVHeap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::UAV, 2046,    0);
		m_CBVHeap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::CBV,  128,    0);
		m_RTVHeap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::RTV,  128,    0);
		m_DSVHeap		= new DescriptorHeap(m_Device.Get(), DescriptorHeapType::DSV,   64,    0);

		for (int i = 0; i < (int)ContextType::MAX; ++i)
		{
			m_CommandQueues[i] = new CommandQueue((ContextType)i, this);
			m_CommandContexts[i] = new CommandContext(m_CommandQueues[i], (ContextType)i, m_Device.Get(), m_GlobalHeap);
		}

		m_Swapchain = new Swapchain(m_CommandQueues[(int)ContextType::Direct]->Get(), m_Factory.Get(), window);
		Gfx::OnPostResourceManagerInit.AddRaw(this, &Device::InitResources);
		Gfx::OnPreResourceManagerShutdown.AddRaw(this, &Device::DestroyResources);
		Gfx::OnPreResourceManagerShutdown.AddRaw(this, &Device::IdleGPU);

		window->OnWindowResize.AddRaw(this, &Device::HandleWindowResize);

		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		m_PresentFence = new Fence(this);

		// ImGui Stuff
		if (flags & Gfx::GfxDeviceFlag::EnableImgui)
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans/NotoSans-Regular.ttf", 18.0f);
			ImGuiStyle& style = ImGui::GetStyle();
			style.ItemInnerSpacing = ImVec2(10.0f, 0.0f);

			DescriptorHandle imguiDescriptor = m_GlobalHeap->AllocatePersistent();

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
		delete m_RTVHeap;
		delete m_DSVHeap;

		if (m_Flags & Gfx::GfxDeviceFlag::EnableImgui)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		for (int i = 0; i < (int)ContextType::MAX; ++i)
		{
			delete m_CommandContexts[i];
			delete m_CommandQueues[i];
		}
		
#if !NO_LOG
		if (m_Flags & Gfx::GfxDeviceFlag::DetailedLogging)
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
		DestroyPSO(m_GenerateMipsPSO);

		DestroyRootSignature(m_GenerateMipsRS);

		delete m_TempBufferAllocator;
		delete m_UploadRingBuffer;
		delete m_Swapchain;
	}

	void Device::Present(bool bEnableVSync)
	{
		CommandContext* context = m_CommandContexts[(int)ContextType::Direct];

		if (m_Flags & Gfx::GfxDeviceFlag::EnableImgui)
		{
			context->BeginEvent("ImGui");

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context->Get());

			context->EndEvent();
		}

		{
			TextureHandle backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = RM_GET(backBufferHandle);
			context->InsertResourceBarrier(backbuffer, D3D12_RESOURCE_STATE_PRESENT);

			context->SubmitResourceBarriers();
		}

		context->Execute();

		m_Swapchain->Present(bEnableVSync);
		m_FrameIndex = m_Swapchain->GetCurrentIndex();

		uint64 presentValue = m_PresentFence->Signal(m_CommandQueues[(int)ContextType::Direct]);
		m_PresentFence->CpuWait(presentValue);

		OnPrepareFrame.Broadcast();

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
			OnReloadShaders.Broadcast();
			m_bNeedsShaderReload = false;
			LB_LOG("Shaders got reloaded. It took %.2fs", t.ElapsedSeconds());
		}

		// Prepare frame render targets
		{
			TextureHandle backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
			Texture* backbuffer = RM_GET(backBufferHandle);
			context->InsertResourceBarrier(backbuffer, D3D12_RESOURCE_STATE_COMMON);

			TextureHandle depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
			Texture* depthBackbuffer = RM_GET(depthBackBufferHandle);
			context->InsertResourceBarrier(depthBackbuffer, D3D12_RESOURCE_STATE_COMMON);

			context->SubmitResourceBarriers();
		}

		if (m_Flags & Gfx::GfxDeviceFlag::EnableImgui)
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

	TextureHandle Device::GetCurrentBackbuffer() const
	{
		return m_Swapchain->GetBackbuffer(m_FrameIndex);
	}

	TextureHandle Device::GetCurrentDepthBackbuffer() const
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

		TextureHandle backBufferHandle = m_Swapchain->GetBackbuffer(m_FrameIndex);
		Texture* backbuffer = RM_GET(backBufferHandle);
		m_CommandContexts[(int)ContextType::Direct]->InsertResourceBarrier(backbuffer, D3D12_RESOURCE_STATE_COMMON);

		TextureHandle depthBackBufferHandle = m_Swapchain->GetDepthBackbuffer(m_FrameIndex);
		Texture* depthBackbuffer = RM_GET(depthBackBufferHandle);
		m_CommandContexts[(int)ContextType::Direct]->InsertResourceBarrier(depthBackbuffer, D3D12_RESOURCE_STATE_COMMON);

		m_CommandContexts[(int)ContextType::Direct]->SubmitResourceBarriers();

		m_GenerateMipsRS = RHI::CreateRootSignature("Generate Mips RS", RSInitializer().Init().AddDescriptorTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV).AddRootConstants(0, 4));

		m_GenerateMipsShader = CreateShader("generatemips.hlsl", "GenerateMip", ShaderType::Compute);
		SC::Compile(m_GenerateMipsShader);

		{
			PipelineStateInitializer psoInit = {};
			psoInit.SetComputeShader(m_GenerateMipsShader);
			psoInit.SetName("Generate Mips PSO");
			psoInit.SetRootSignature(m_GenerateMipsRS);
			m_GenerateMipsPSO = CreatePSO(psoInit);
		}

		m_UploadRingBuffer = new RingBufferAllocator(m_CommandQueues[(int)ContextType::Copy], Utils::ToMB(256), "Upload Ring Buffer");
		m_TempBufferAllocator = new RingBufferAllocator(m_CommandQueues[(int)ContextType::Copy], Utils::ToMB(4), "Temp Buffers Ring Buffer");
	}

	DescriptorHandle Device::AllocatePersistent(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV:
			return m_GlobalHeap->AllocatePersistent();
		case DescriptorHeapType::RTV:
			return m_RTVHeap->AllocatePersistent();
		case DescriptorHeapType::DSV:
			return m_DSVHeap->AllocatePersistent();
		case DescriptorHeapType::UAV:
			return m_UAVHeap->AllocatePersistent();
		case DescriptorHeapType::CBV:
			return m_CBVHeap->AllocatePersistent();
		default:
			ensure(false);
			return DescriptorHandle();
		}
	}

	DescriptorHandle Device::AllocateTemp(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV:
			return m_GlobalHeap->AllocateTemp();
		case DescriptorHeapType::RTV:
			return m_RTVHeap->AllocateTemp();
		case DescriptorHeapType::DSV:
			return m_DSVHeap->AllocateTemp();
		case DescriptorHeapType::UAV:
			return m_UAVHeap->AllocateTemp();
		case DescriptorHeapType::CBV:
			return m_CBVHeap->AllocateTemp();
		default:
			ensure(false);
			return DescriptorHandle();
		}
	}

	void Device::FreeHandle(DescriptorHandle& handle)
	{
		switch (handle.OwnerHeapType)
		{
		case DescriptorHeapType::SRV:
			m_GlobalHeap->FreePersistent(handle);
			break;
		case DescriptorHeapType::RTV:
			m_RTVHeap->FreePersistent(handle);
			break;
		case DescriptorHeapType::DSV:
			m_DSVHeap->FreePersistent(handle);
			break;
		case DescriptorHeapType::UAV:
			m_UAVHeap->FreePersistent(handle);
			break;
		case DescriptorHeapType::CBV:
			m_CBVHeap->FreePersistent(handle);
			break;
		}
	}

	void Device::CreateSRV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel)
	{
		DXGI_FORMAT format;
		switch (texture->Spec.Format)
		{
		case Format::D16_UNORM:
			format = DXGI_FORMAT_R16_UNORM;
			break;
		case Format::D32_SFLOAT:
			format = DXGI_FORMAT_R32_FLOAT;
			break;
		case Format::D32_SFLOAT_S8_UINT:
			format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		default:
			format = D3DFormat(texture->Spec.Format);
			break;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = format,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		uint32 mostDetailedMip = mipLevel;
		uint32 mipLevels = mipLevel == 0 ? ~0 : 1;

		if (texture->Spec.Type == TextureType::Texture1D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvDesc.Texture1D = {
				.MostDetailedMip = mostDetailedMip,
				.MipLevels = mipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (texture->Spec.Type == TextureType::Texture2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D = {
				.MostDetailedMip = mostDetailedMip,
				.MipLevels = mipLevels,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (texture->Spec.Type == TextureType::Texture3D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D = {
				.MostDetailedMip = mostDetailedMip,
				.MipLevels = mipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else if (texture->Spec.Type == TextureType::TextureCube)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube = {
				.MostDetailedMip = mostDetailedMip,
				.MipLevels = mipLevels,
				.ResourceMinLODClamp = 0.0f
			};
		}

		m_Device->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, descriptorHandle.CpuHandle);
	}

	void Device::CreateUAV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		DXGI_FORMAT format = D3DFormat(texture->Spec.Format);
		uavDesc.Format = format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM : format;

		if (texture->Spec.Type == TextureType::Texture1D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavDesc.Texture1D = {
				.MipSlice = mipLevel
			};
		}
		else if (texture->Spec.Type == TextureType::Texture2D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D = {
				.MipSlice = mipLevel,
				.PlaneSlice = 0
			};
		}
		else if (texture->Spec.Type == TextureType::Texture3D)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = mipLevel;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = -1;
		}
		else if (texture->Spec.Type == TextureType::TextureCube)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray = {
				.MipSlice = mipLevel,
				.FirstArraySlice = 0,
				.ArraySize = 6,
				.PlaneSlice = 0
			};
		}

		m_Device->CreateUnorderedAccessView(texture->Resource.Get(), nullptr, &uavDesc, descriptorHandle.CpuHandle);
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

		SYSTEMTIME now;
		GetLocalTime(&now);
		filename.append(std::format(L"{}_{}_{}_{}_{}_{}", now.wDay, now.wMonth, now.wYear, now.wHour, now.wMinute, now.wSecond));

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

	void Device::GenerateMipLevels(TextureHandle texture)
	{
		Texture* pTexture = RM_GET(texture);
		FAILIF(!pTexture);

		CommandContext* cmd = m_CommandContexts[(int)ContextType::Direct];

		bool bIsRGB = pTexture->Spec.Format == Format::RGBA8_UNORM_SRGB;

		cmd->SetPipelineState(m_GenerateMipsPSO);
		for (uint16 i = 1; i < pTexture->Spec.MipLevels; ++i)
		{
			uint2 outputMipSize = { pTexture->Spec.Width , pTexture->Spec.Height };
			outputMipSize.x >>= i;
			outputMipSize.y >>= i;
			float2 texelSize = { 1.0f / outputMipSize.x, 1.0f / outputMipSize.y };

			cmd->BindConstants(1, 0, texelSize);
			cmd->BindConstants(1, 2, bIsRGB ? 1 : 0);

			DescriptorHandle srvHandles = m_GlobalHeap->AllocateTemp();
			CreateSRV(pTexture, srvHandles, i - 1);
			cmd->BindConstants(1, 3, srvHandles.Index);

			DescriptorHandle uavHandles[] = { pTexture->UAVHandle[i] };
			cmd->BindTempDescriptorTable(0, uavHandles, 1);
			cmd->Dispatch(Math::Max(outputMipSize.x / 8, 1u), Math::Max(outputMipSize.y / 8, 1u), 1);
			cmd->InsertUAVBarrier(pTexture);
		}

		cmd->InsertResourceBarrier(texture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, pTexture->Spec.MipLevels - 1);
		cmd->SubmitResourceBarriers();
	}

	void Device::PickGPU()
	{
		m_Adapter = nullptr;

		// Get IDXGIFactory6
		RefCountPtr<IDXGIFactory6> factory6;
		DX_CHECK(m_Factory->QueryInterface(IID_PPV_ARGS(factory6.ReleaseAndGetAddressOf())));
		for (uint32_t adapterIndex = 0;
			!FAILED(factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(m_Adapter.ReleaseAndGetAddressOf())));
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
			RefCountPtr<IUnknown> GraphicsAnalysis;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, GraphicsAnalysisID, GraphicsAnalysis.GetRefVoid())))
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
			RefCountPtr<IUnknown> renderDoc;
			if (SUCCEEDED(m_Device->QueryInterface(renderDocID, renderDoc.GetRefVoid())))
			{
				// Running under RenderDoc, so enable capturing mode
				LB_LOG("Running under Render Doc, ID3D12InfoQueue features won't be enabled!");
				return true;
			}
		}

		return false;
	}
}

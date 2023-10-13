#pragma once

#include "definitions.h"
#include "core/array.h"
#include "core/refcountptr.h"
#include "rootsignature.h"
#include "shader.h"
#include "pipelinestateobject.h"
#include "texture.h"

#include <dxgi1_6.h>

namespace limbo::Core
{
	class Window;
}

namespace limbo::RHI
{
	enum class DescriptorHeapType : uint8;
	class RingBufferAllocator;
	struct DescriptorHandle;
	class DescriptorHeap;
	class CommandQueue;
	class CommandContext;
	struct WindowInfo;
	struct DrawInfo;
	class Swapchain;
	class Fence;

	typedef uint8 GfxDeviceFlags;

	struct GPUInfo
	{
		char Name[128];
		bool bSupportsRaytracing;
	};

	class Device
	{
		using CommandQueueList   = TStaticArray<CommandQueue*, (int)ContextType::MAX>;
		using CommandContextList = TStaticArray<CommandContext*, (int)ContextType::MAX>;

	private:
		RefCountPtr<IDXGIFactory2>			m_Factory;
		RefCountPtr<IDXGIAdapter1>			m_Adapter;
		RefCountPtr<ID3D12Device5>			m_Device;

		CommandContextList					m_CommandContexts;
		CommandQueueList					m_CommandQueues;
		Fence*								m_PresentFence;

		RingBufferAllocator*				m_UploadRingBuffer;
		RingBufferAllocator*				m_TempBufferAllocator;

		CD3DX12FeatureSupport				m_FeatureSupport;

		Swapchain*							m_Swapchain;
		uint32								m_FrameIndex;

		DescriptorHeap*						m_GlobalHeap;
		DescriptorHeap*						m_UAVHeap;
		DescriptorHeap*						m_CBVHeap;
		DescriptorHeap*						m_SRVHeap;
		DescriptorHeap*						m_DSVHeap;
		DescriptorHeap*						m_RTVHeap;

		GfxDeviceFlags						m_Flags;
		bool								m_bNeedsResize = false;
		bool								m_bNeedsShaderReload = false;

		bool								m_bPIXCanCapture = false;
		std::wstring						m_LastGPUCaptureFilename;

		GPUInfo								m_GPUInfo;

		RootSignatureHandle					m_GenerateMipsRS;
		ShaderHandle						m_GenerateMipsShader;
		PSOHandle							m_GenerateMipsPSO;

	public:
		static Device* Ptr;

		Device(Core::Window* window, GfxDeviceFlags flags);
		~Device();

		void Present(bool bEnableVSync);

		void HandleWindowResize(uint32 width, uint32 height);

		// This will capture the next frame and create a .wpix file, in the root directory, with a random name
		void TakeGPUCapture();
		void OpenLastGPUCapture();

		uint32 GetCurrentFrameIndex() const
		{
			return m_FrameIndex;
		}

		const GPUInfo& GetGPUInfo() const
		{
			return m_GPUInfo;
		}

		RingBufferAllocator* GetRingBufferAllocator() const
		{
			return m_UploadRingBuffer;
		}

		RingBufferAllocator* GetTempBufferAllocator() const
		{
			return m_TempBufferAllocator;
		}

		TextureHandle GetCurrentBackbuffer() const;
		TextureHandle GetCurrentDepthBackbuffer() const;
		Format GetSwapchainFormat();
		Format GetSwapchainDepthFormat();

		bool CanTakeGPUCapture();

		DescriptorHandle AllocatePersistent(DescriptorHeapType heapType);
		DescriptorHandle AllocateTemp(DescriptorHeapType heapType);
		void FreeHandle(DescriptorHandle& handle);

		void CreateSRV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel = 0);
		void CreateUAV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel = 0);

		uint32 GetBackbufferWidth();
		uint32 GetBackbufferHeight();

		void MarkReloadShaders();

		void GenerateMipLevels(TextureHandle texture);

		// D3D12 specific
		ID3D12Device5* GetDevice() const { return m_Device.Get(); }
		CommandContext* GetCommandContext(ContextType type) const { return m_CommandContexts[(int)type]; }
		CommandQueue* GetCommandQueue(ContextType type) const { return m_CommandQueues[(int)type]; }
		Fence* GetPresentFence() const { return m_PresentFence; }

	private:
		void InitResources();
		void DestroyResources();

		void PickGPU();
		void IdleGPU();

		bool IsUnderPIX();
		bool IsUnderRenderDoc();
	};

	//
	// Global Device
	//

	FORCEINLINE const GPUInfo& GetGPUInfo()
	{
		return Device::Ptr->GetGPUInfo();
	}

	FORCEINLINE RingBufferAllocator* GetRingBufferAllocator()
	{
		return Device::Ptr->GetRingBufferAllocator();
	}

	FORCEINLINE RingBufferAllocator* GetTempBufferAllocator()
	{
		return Device::Ptr->GetTempBufferAllocator();
	}

	FORCEINLINE void ReloadShaders()
	{
		Device::Ptr->MarkReloadShaders();
	}

	FORCEINLINE void Present(bool bEnableVSync)
	{
		Device::Ptr->Present(bEnableVSync);
	}

	FORCEINLINE Format GetSwapchainFormat()
	{
		return Device::Ptr->GetSwapchainFormat();
	}

	FORCEINLINE TextureHandle GetCurrentBackbuffer()
	{
		return Device::Ptr->GetCurrentBackbuffer();
	}

	FORCEINLINE TextureHandle GetCurrentDepthBackbuffer()
	{
		return Device::Ptr->GetCurrentDepthBackbuffer();
	}

	FORCEINLINE uint32 GetBackbufferWidth()
	{
		return Device::Ptr->GetBackbufferWidth();
	}

	FORCEINLINE uint32 GetBackbufferHeight()
	{
		return Device::Ptr->GetBackbufferHeight();
	}

	FORCEINLINE bool CanTakeGPUCapture()
	{
		return Device::Ptr->CanTakeGPUCapture();
	}

	FORCEINLINE void TakeGPUCapture()
	{
		Device::Ptr->TakeGPUCapture();
	}

	FORCEINLINE void OpenLastGPUCapture()
	{
		Device::Ptr->OpenLastGPUCapture();
	}
}

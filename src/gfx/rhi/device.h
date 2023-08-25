#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "resourcemanager.h"
#include "commandcontext.h"
#include "shaderbindingtable.h"

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	enum class DescriptorHeapType : uint8;
	class RingBufferAllocator;
	struct DescriptorHandle;
	class DescriptorHeap;
	class CommandQueue;
	struct WindowInfo;
	struct DrawInfo;
	class Swapchain;
	class Buffer;
	class Texture;
	class Shader;
	class Fence;

	typedef uint8 GfxDeviceFlags;

	struct GPUInfo
	{
		char Name[128];
	};

	class Device
	{
		using CommandQueueList   = std::array<CommandQueue*, (int)ContextType::MAX>;
		using CommandContextList = std::array<CommandContext*, (int)ContextType::MAX>;

	private:
		ComPtr<IDXGIFactory2>				m_Factory;
		ComPtr<IDXGIAdapter1>				m_Adapter;
		ComPtr<ID3D12Device5>				m_Device;

		CommandContextList					m_CommandContexts;
		CommandQueueList					m_CommandQueues;
		Fence*								m_PresentFence;

		RingBufferAllocator*				m_UploadRingBuffer;

		CD3DX12FeatureSupport				m_FeatureSupport;

		Swapchain*							m_Swapchain;
		uint32								m_FrameIndex;

		DescriptorHeap*						m_GlobalHeap;
		DescriptorHeap*						m_Dsvheap;
		DescriptorHeap*						m_Rtvheap;

		GfxDeviceFlags						m_Flags;
		bool								m_bNeedsResize = false;
		bool								m_bNeedsShaderReload = false;

		bool								m_bPIXCanCapture = false;
		std::wstring						m_LastGPUCaptureFilename;

		GPUInfo								m_GPUInfo;

		Handle<Shader>						m_GenerateMipsShader;

	public:
		static Device* Ptr;

		DECLARE_MULTICAST_DELEGATE(TOnResizedSwapchain, uint32, uint32);
		TOnResizedSwapchain OnResizedSwapchain;

		DECLARE_MULTICAST_DELEGATE(TReloadShaders);
		TReloadShaders ReloadShaders;

		DECLARE_MULTICAST_DELEGATE(TOnPrepareFrame);
		TOnPrepareFrame OnPrepareFrame;

	public:
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

		Handle<Texture> GetCurrentBackbuffer() const;
		Handle<Texture> GetCurrentDepthBackbuffer() const;
		Format GetSwapchainFormat();
		Format GetSwapchainDepthFormat();

		bool CanTakeGPUCapture();

		// D3D12 specific
		ID3D12Device5* GetDevice() const { return m_Device.Get(); }
		CommandContext* GetCommandContext(ContextType type) const { return m_CommandContexts.at((int)type); }
		CommandQueue* GetCommandQueue(ContextType type) const { return m_CommandQueues.at((int)type); }
		Fence* GetPresentFence() const { return m_PresentFence; }

		DescriptorHandle AllocateHandle(DescriptorHeapType heapType);
		DescriptorHandle AllocateTempHandle(DescriptorHeapType heapType);
		void FreeHandle(DescriptorHandle& handle);

		void CreateSRV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel = 0);
		void CreateUAV(Texture* texture, DescriptorHandle& descriptorHandle, uint8 mipLevel = 0);

		uint32 GetBackbufferWidth();
		uint32 GetBackbufferHeight();

		void MarkReloadShaders();

		void GenerateMipLevels(Handle<Texture> texture);

	private:
		void InitResources();
		void DestroyResources();

		void PickGPU();
		void IdleGPU();

		bool IsUnderPIX();
		bool IsUnderRenderDoc();
	};

	inline const GPUInfo& GetGPUInfo()
	{
		return Device::Ptr->GetGPUInfo();
	}

	inline RingBufferAllocator* GetRingBufferAllocator()
	{
		return Device::Ptr->GetRingBufferAllocator();
	}

	inline CommandContext* GetCommandContext(ContextType type = ContextType::Direct)
	{
		return Device::Ptr->GetCommandContext(type);
	}

	inline void CopyTextureToBackBuffer(Handle<Texture> texture)
	{
		GetCommandContext()->CopyTextureToBackBuffer(texture);
	}

	inline void CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset = 0, uint64 dstOffset = 0)
	{
		GetCommandContext()->CopyBufferToBuffer(src, dst, numBytes, srcOffset, dstOffset);
	}

	inline void CopyTextureToTexture(Handle<Texture> src, Handle<Texture> dst)
	{
		GetCommandContext()->CopyTextureToTexture(src, dst);
	}

	inline void CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst)
	{
		GetCommandContext()->CopyBufferToTexture(src, dst);
	}

	inline void BeginEvent(const char* name, uint64 color = 0)
	{
		GetCommandContext()->BeginEvent(name, color);
	}

	inline void ScopedEvent(const char* name, uint64 color = 0)
	{
		GetCommandContext()->ScopedEvent(name, color);
	}

	inline void ReloadShaders()
	{
		Device::Ptr->MarkReloadShaders();
	}

	inline void EndEvent()
	{
		GetCommandContext()->EndEvent();
	}

	inline void TakeGPUCapture()
	{
		Device::Ptr->TakeGPUCapture();
	}

	inline void OpenLastGPUCapture()
	{
		Device::Ptr->OpenLastGPUCapture();
	}

	inline void BindVertexBuffer(Handle<Buffer> buffer)
	{
		GetCommandContext()->BindVertexBuffer(buffer);
	}

	inline void BindIndexBuffer(Handle<Buffer> buffer)
	{
		GetCommandContext()->BindIndexBuffer(buffer);
	}

	inline void BindVertexBufferView(VertexBufferView view)
	{
		GetCommandContext()->BindVertexBufferView(view);
	}

	inline void BindIndexBufferView(IndexBufferView view)
	{
		GetCommandContext()->BindIndexBufferView(view);
	}

	inline void BindShader(Handle<Shader> shader)
	{
		GetCommandContext()->BindShader(shader);
	}

	inline void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0)
	{
		GetCommandContext()->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 baseVertex = 0, uint32 firstInstance = 0)
	{
		GetCommandContext()->DrawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	inline void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		GetCommandContext()->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	inline void DispatchRays(const ShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth = 1)
	{
		GetCommandContext()->DispatchRays(sbt, width, height, depth);
	}

	inline void Present(bool bEnableVSync)
	{
		Device::Ptr->Present(bEnableVSync);
	}

	inline Format GetSwapchainFormat()
	{
		return Device::Ptr->GetSwapchainFormat();
	}

	inline uint32 GetBackbufferWidth()
	{
		return Device::Ptr->GetBackbufferWidth();
	}

	inline uint32 GetBackbufferHeight()
	{
		return Device::Ptr->GetBackbufferHeight();
	}

	inline uint16 CalculateMipCount(uint32 width, uint32 height = 0, uint32 depth = 0)
	{
		uint16 mipCount = 0;
		uint32 mipSize = Math::Max(width, Math::Max(height, depth));
		while (mipSize >= 1) { mipSize >>= 1; mipCount++; }
		return mipCount;
	}

	inline void GenerateMipLevels(Handle<Texture> texture)
	{
		Device::Ptr->GenerateMipLevels(texture);
	}

	inline bool CanTakeGPUCapture()
	{
		return Device::Ptr->CanTakeGPUCapture();
	}
}

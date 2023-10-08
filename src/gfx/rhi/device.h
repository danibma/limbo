#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "resourcemanager.h"
#include "commandcontext.h"
#include "shaderbindingtable.h"
#include "gfx/profiler.h"
#include "core/array.h"
#include "core/refcountptr.h"
#include "buffer.h"
#include "texture.h"

namespace limbo::Core
{
	class Window;
}

namespace limbo::RHI
{
	enum class DescriptorHeapType : uint8;
	class RingBufferAllocator;
	class PipelineStateObject;
	struct DescriptorHandle;
	class DescriptorHeap;
	class RootSignature;
	class CommandQueue;
	struct WindowInfo;
	struct DrawInfo;
	struct Shader;
	class Swapchain;
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

		RootSignature*						m_GenerateMipsRS;
		ShaderHandle						m_GenerateMipsShader;
		PipelineStateObject*				m_GenerateMipsPSO;

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
		CommandContext* GetCommandContext(ContextType type) const { return m_CommandContexts.at((int)type); }
		CommandQueue* GetCommandQueue(ContextType type) const { return m_CommandQueues.at((int)type); }
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

	FORCEINLINE CommandContext* GetCommandContext(ContextType type = ContextType::Direct)
	{
		return Device::Ptr->GetCommandContext(type);
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

	FORCEINLINE void GenerateMipLevels(TextureHandle texture)
	{
		Device::Ptr->GenerateMipLevels(texture);
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

	//
	// Global Command Context
	//
	FORCEINLINE void CopyTextureToBackBuffer(TextureHandle texture)
	{
		GetCommandContext()->CopyTextureToBackBuffer(texture);
	}

	FORCEINLINE void CopyBufferToBuffer(BufferHandle src, BufferHandle dst, uint64 numBytes, uint64 srcOffset = 0, uint64 dstOffset = 0)
	{
		GetCommandContext()->CopyBufferToBuffer(src, dst, numBytes, srcOffset, dstOffset);
	}

	FORCEINLINE void CopyTextureToTexture(TextureHandle src, TextureHandle dst)
	{
		GetCommandContext()->CopyTextureToTexture(src, dst);
	}

	FORCEINLINE void CopyBufferToTexture(BufferHandle src, TextureHandle dst)
	{
		GetCommandContext()->CopyBufferToTexture(src, dst);
	}

	FORCEINLINE void BeginProfileEvent(const char* name, uint64 color = 0, ContextType type = ContextType::Direct)
	{
		GetCommandContext(type)->BeginEvent(name, color);
		PROFILE_BEGIN(GetCommandContext(type), name);
	}

	FORCEINLINE void EndProfileEvent(const char* name, ContextType type = ContextType::Direct)
	{
		PROFILE_END(GetCommandContext(type), name);
		GetCommandContext(type)->EndEvent();
	}

	FORCEINLINE void BeginEvent(const char* name, uint64 color = 0)
	{
		GetCommandContext()->BeginEvent(name, color);
	}

	FORCEINLINE void ScopedEvent(const char* name, uint64 color = 0)
	{
		GetCommandContext()->ScopedEvent(name, color);
	}

	FORCEINLINE void EndEvent()
	{
		GetCommandContext()->EndEvent();
	}

	FORCEINLINE void ClearRenderTargets(Span<TextureHandle> renderTargets, float4 color = float4(0.0f))
	{
		GetCommandContext()->ClearRenderTargets(renderTargets, color);
	}

	FORCEINLINE void ClearDepthTarget(TextureHandle depthTarget, float depth = 1.0f, uint8 stencil = 0)
	{
		GetCommandContext()->ClearDepthTarget(depthTarget, depth, stencil);
	}

	FORCEINLINE void SetVertexBuffer(BufferHandle buffer)
	{
		GetCommandContext()->SetVertexBuffer(buffer);
	}

	FORCEINLINE void SetIndexBuffer(BufferHandle buffer)
	{
		GetCommandContext()->SetIndexBuffer(buffer);
	}

	FORCEINLINE void SetVertexBufferView(VertexBufferView view)
	{
		GetCommandContext()->SetVertexBufferView(view);
	}

	FORCEINLINE void SetIndexBufferView(IndexBufferView view)
	{
		GetCommandContext()->SetIndexBufferView(view);
	}

	FORCEINLINE void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		GetCommandContext()->SetPrimitiveTopology(topology);
	}

	FORCEINLINE void SetViewport(uint32 width, uint32 height, float topLeft = 0.0f, float topRight = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f)
	{
		GetCommandContext()->SetViewport(width, height, topLeft, topRight, minDepth, maxDepth);
	}

	FORCEINLINE void SetPipelineState(PipelineStateObject* pso)
	{
		GetCommandContext()->SetPipelineState(pso);
	}

	FORCEINLINE void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthTarget = TextureHandle())
	{
		GetCommandContext()->SetRenderTargets(renderTargets, depthTarget);
	}

	FORCEINLINE void BindTempDescriptorTable(uint32 rootParameter, DescriptorHandle* handles, uint32 count)
	{
		GetCommandContext()->BindDescriptorTable(rootParameter, handles, count);
	}

	template<typename T>
	FORCEINLINE void BindTempConstantBuffer(uint32 rootParameter, const T& data)
	{
		GetCommandContext()->BindTempConstantBuffer(rootParameter, &data, sizeof(T));
	}

	FORCEINLINE void BindRootSRV(uint32 rootParameter, uint64 gpuVirtualAddress)
	{
		GetCommandContext()->BindRootSRV(rootParameter, gpuVirtualAddress);
	}

	FORCEINLINE void BindRootCBV(uint32 rootParameter, uint64 gpuVirtualAddress)
	{
		GetCommandContext()->BindRootCBV(rootParameter, gpuVirtualAddress);
	}

	template<typename T>
	FORCEINLINE void BindConstants(uint32 rootParameter, uint32 offsetIn32bits, const T& data)
	{
		static_assert(sizeof(T) % sizeof(uint32) == 0);
		GetCommandContext()->BindConstants(rootParameter, sizeof(T) / sizeof(uint32), offsetIn32bits, &data);
	}

	FORCEINLINE void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0)
	{
		GetCommandContext()->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	FORCEINLINE void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 baseVertex = 0, uint32 firstInstance = 0)
	{
		GetCommandContext()->DrawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	FORCEINLINE void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		GetCommandContext()->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	FORCEINLINE void DispatchRays(const ShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth = 1)
	{
		GetCommandContext()->DispatchRays(sbt, width, height, depth);
	}
}

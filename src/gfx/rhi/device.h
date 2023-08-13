#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "resourcemanager.h"

#include <vector>

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	enum class DescriptorHeapType : uint8;
	struct DescriptorHandle;
	class DescriptorHeap;
	struct WindowInfo;
	struct DrawInfo;
	class Swapchain;
	class Buffer;
	class Texture;
	class Shader;

	typedef uint8 GfxDeviceFlags;

	struct GPUInfo
	{
		char Name[128];
	};

	class Device
	{
		ComPtr<IDXGIFactory2>				m_Factory;
		ComPtr<IDXGIAdapter1>				m_Adapter;
		ComPtr<ID3D12Device>				m_Device;

		ComPtr<ID3D12CommandAllocator>		m_CommandAllocators[NUM_BACK_BUFFERS];
		ComPtr<ID3D12GraphicsCommandList>	m_CommandList;
		ComPtr<ID3D12CommandQueue>			m_CommandQueue;

		ComPtr<ID3D12Fence>					m_Fence;
		HANDLE								m_FenceEvent;
		uint64								m_FenceValues[3] = { 0, 0, 0 };

		Swapchain*							m_Swapchain;
		uint32								m_FrameIndex;

		DescriptorHeap*						m_Srvheap;
		DescriptorHeap*						m_Dsvheap;
		DescriptorHeap*						m_Rtvheap;
		DescriptorHeap*						m_Samplerheap;

		std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;

		GfxDeviceFlags						m_Flags;
		bool								m_bNeedsResize = false;
		bool								m_bNeedsShaderReload = false;

		bool								m_bPIXCanCapture = false;
		std::wstring						m_LastGPUCaptureFilename;

		GPUInfo								m_GPUInfo;

		Handle<Shader>						m_BoundShader;
		Handle<Shader>						m_GenerateMipsShader;

	public:
		static Device* Ptr;

		DECLARE_MULTICAST_DELEGATE(TOnResizedSwapchain, uint32, uint32);
		TOnResizedSwapchain OnResizedSwapchain;

		DECLARE_MULTICAST_DELEGATE(TReloadShaders);
		TReloadShaders ReloadShaders;

	public:
		Device(Core::Window* window, GfxDeviceFlags flags);
		~Device();

		void CopyTextureToBackBuffer(Handle<Texture> texture);
		void CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst);
		void CopyBufferToTexture(Buffer* src, Texture* dst);
		void CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);
		void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);

		template<typename T1, typename T2>
		void CopyResource(Handle<T1> src, Handle<T2> dst)
		{
			static_assert(std::_Is_any_of_v<T1, Texture, Buffer>);
			static_assert(std::_Is_any_of_v<T2, Texture, Buffer>);

			Texture* dstResource = ResourceManager::Ptr->GetTexture(dst);
			FAILIF(!dstResource);
			Buffer* srcResource = ResourceManager::Ptr->GetBuffer(src);
			FAILIF(!srcResource);

			m_CommandList->CopyResource(dstResource->Resource.Get(), srcResource->Resource.Get());
		}

		void BeginEvent(const char* name, uint64 color = 0);
		void EndEvent();
		void ScopedEvent(const char* name, uint64 color = 0);
		// This will capture the next frame and create a .wpix file, in the root directory, with a random name
		void TakeGPUCapture();
		void OpenLastGPUCapture();

		void BindVertexBuffer(Handle<Buffer> buffer);
		void BindIndexBuffer(Handle<Buffer> buffer);
		void BindShader(Handle<Shader> shader);
		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
		void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance);

		void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);

		void Present(bool bEnableVSync);

		void HandleWindowResize(uint32 width, uint32 height);

		uint32 GetCurrentFrameIndex() const
		{
			return m_FrameIndex;
		}

		const GPUInfo& GetGPUInfo() const
		{
			return m_GPUInfo;
		}

		Format GetSwapchainFormat();
		Format GetSwapchainDepthFormat();

		bool CanTakeGPUCapture();

		// D3D12 specific
		ID3D12Device* GetDevice() const { return m_Device.Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }

		DescriptorHandle AllocateHandle(DescriptorHeapType heapType);

		void TransitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel = ~0);
		void TransitionResource(Texture* texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel = ~0);

		void TransitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState);
		void TransitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState);

		uint32 GetBackbufferWidth();
		uint32 GetBackbufferHeight();

		void MarkReloadShaders();

		void GenerateMipLevels(Handle<Texture> texture);

	private:
		void InitResources();
		void DestroyResources();

		void PickGPU();
		void WaitGPU();

		void NextFrame();

		bool IsUnderPIX();
		bool IsUnderRenderDoc();

		void SubmitResourceBarriers();

		void InstallDrawState();
		void BindSwapchainRenderTargets();
	};

	inline const GPUInfo& GetGPUInfo()
	{
		return Device::Ptr->GetGPUInfo();
	}

	inline void CopyTextureToBackBuffer(Handle<Texture> texture)
	{
		Device::Ptr->CopyTextureToBackBuffer(texture);
	}

	inline void CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset = 0, uint64 dstOffset = 0)
	{
		Device::Ptr->CopyBufferToBuffer(src, dst, numBytes, srcOffset, dstOffset);
	}

	template<typename T1, typename T2>
	void CopyResource(Handle<T1> src, Handle<T2> dst)
	{
		Device::Ptr->CopyResource(src, dst);
	}

	inline void CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst)
	{
		Device::Ptr->CopyBufferToTexture(src, dst);
	}

	inline void BeginEvent(const char* name, uint64 color = 0)
	{
		Device::Ptr->BeginEvent(name, color);
	}

	inline void ScopedEvent(const char* name, uint64 color = 0)
	{
		Device::Ptr->ScopedEvent(name, color);
	}

	inline void ReloadShaders()
	{
		Device::Ptr->MarkReloadShaders();
	}

	inline void EndEvent()
	{
		Device::Ptr->EndEvent();
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
		Device::Ptr->BindVertexBuffer(buffer);
	}

	inline void BindIndexBuffer(Handle<Buffer> buffer)
	{
		Device::Ptr->BindIndexBuffer(buffer);
	}

	inline void BindShader(Handle<Shader> shader)
	{
		Device::Ptr->BindShader(shader);
	}

	inline void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0)
	{
		Device::Ptr->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 baseVertex = 0, uint32 firstInstance = 0)
	{
		Device::Ptr->DrawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	inline void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		Device::Ptr->Dispatch(groupCountX, groupCountY, groupCountZ);
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

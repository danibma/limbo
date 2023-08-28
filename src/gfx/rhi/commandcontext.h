#pragma once

#include "resourcepool.h"
#include "resourcemanager.h"
#include "shaderbindingtable.h"

namespace limbo::Gfx
{
	constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN = (D3D12_RESOURCE_STATES)-1;

	struct ResourceState
	{
		ResourceState()
		{
			for (uint8 i = 0; i < D3D12_REQ_MIP_LEVELS; ++i)
				m_States[i] = D3D12_RESOURCE_STATE_UNKNOWN;
		}

		D3D12_RESOURCE_STATES& Get(uint32 index)
		{
			return m_States[index == ~0ul ? 0 : index];
		}

		void Set(D3D12_RESOURCE_STATES newState, uint32 subresource)
		{
			if (Get(subresource) == newState) return;

			if (subresource == ~0)
			{
				for (uint8 i = 0; i < D3D12_REQ_MIP_LEVELS; ++i)
					m_States[i] = newState;
			}
			else
			{
				m_States[subresource] = newState;
			}
		}

	private:
		std::array<D3D12_RESOURCE_STATES, D3D12_REQ_MIP_LEVELS> m_States;
	};

	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class CommandContext
	{
		using ResourceStatesMap = std::unordered_map<const void*, ResourceState>;

	private:
		bool								m_bIsClosed = false;

		ID3D12CommandAllocator*				m_Allocator;
		ComPtr<ID3D12GraphicsCommandList4>	m_CommandList;
		CommandQueue*						m_ParentQueue;
		ContextType							m_Type;

		DescriptorHeap*						m_GlobalHeap;

		ResourceStatesMap					m_ResourceStates;
		std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;

		Handle<Shader>						m_BoundShader;

	public:
		CommandContext(CommandQueue* queue, ContextType type, ID3D12Device5* device, DescriptorHeap* globalHeap);
		~CommandContext();

		CommandQueue* GetQueue() const { return m_ParentQueue; }
		ID3D12GraphicsCommandList4* Get() const { return m_CommandList.Get(); }
		ContextType GetType() const { return m_Type; }

		void CopyTextureToTexture(Handle<Texture> src, Handle<Texture> dst);
		void CopyTextureToBackBuffer(Handle<Texture> texture);
		void CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst, uint64 dstOffset = 0);
		void CopyBufferToTexture(Buffer* src, Texture* dst, uint64 dstOffset = 0);
		void CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);
		void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);

		void BeginEvent(const char* name, uint64 color = 0);
		void EndEvent();
		void ScopedEvent(const char* name, uint64 color = 0);

		void BindSwapchainRenderTargets();
		void BindVertexBuffer(Handle<Buffer> buffer);
		void BindIndexBuffer(Handle<Buffer> buffer);
		void BindVertexBufferView(VertexBufferView view);
		void BindIndexBufferView(IndexBufferView view);
		void BindShader(Handle<Shader> shader);

		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
		void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance);
		void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);
		void DispatchRays(const ShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth);

		void BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, Handle<Buffer> scratch, Handle<Buffer> result);

		void Reset();
		void Free(uint64 fenceValue);
		uint64 Execute();

		template<typename ResourceType>
		void InsertResourceBarrier(ResourceType* resource, D3D12_RESOURCE_STATES newState, uint32 subresource = ~0)
		{
			ResourceState& resourceState = m_ResourceStates[resource];
			D3D12_RESOURCE_STATES oldState = resourceState.Get(subresource);

			// Common state promotion - https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#common-state-promotion
			if (oldState == D3D12_RESOURCE_STATE_UNKNOWN || resource->bResetState)
			{
				resourceState.Set(newState, subresource);
				resource->bResetState = false;
			}
			else
			{
				if (oldState == newState) return;
				if (oldState == D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE && newState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) return;

				check(IsTransitionAllowed(oldState));
				check(IsTransitionAllowed(newState));

				resourceState.Set(newState, subresource);

				m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource->Resource.Get(), oldState, newState, subresource));
			}
		}

		template<typename ResourceType>
		void InsertResourceBarrier(Handle<ResourceType> resource, D3D12_RESOURCE_STATES newState, uint32 subresource = ~0)
		{
			InsertResourceBarrier(GetResource(resource), newState, subresource);
		}

		template<typename ResourceType>
		void InsertUAVBarrier(ResourceType* resource)
		{
			m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(resource->Resource.Get()));
		}

		template<typename ResourceType>
		void InsertUAVBarrier(Handle<ResourceType> resource)
		{
			InsertUAVBarrier(GetResource(resource));
		}

		void SubmitResourceBarriers();

	private:
		void InstallDrawState();
		bool IsTransitionAllowed(D3D12_RESOURCE_STATES state);
	};
}

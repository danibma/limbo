#pragma once

#include "resourcepool.h"
#include "resourcemanager.h"
#include "shaderbindingtable.h"

namespace limbo::Gfx
{
	class Device;
	class CommandQueue;
	class DescriptorHeap;
	class CommandContext
	{
		ID3D12CommandAllocator*				m_Allocator;
		ComPtr<ID3D12GraphicsCommandList4>	m_CommandList;
		CommandQueue*						m_ParentQueue;
		ContextType							m_Type;

		DescriptorHeap*						m_GlobalHeap;

		std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;

		Handle<Shader>						m_BoundShader;

	public:
		CommandContext(CommandQueue* queue, ContextType type, ID3D12Device5* device, DescriptorHeap* globalHeap);
		~CommandContext();

		ID3D12GraphicsCommandList4* Get() const { return m_CommandList.Get(); }

		void CopyTextureToTexture(Handle<Texture> src, Handle<Texture> dst);
		void CopyTextureToBackBuffer(Handle<Texture> texture);
		void CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst);
		void CopyBufferToTexture(Buffer* src, Texture* dst);
		void CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);
		void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset);

		void BeginEvent(const char* name, uint64 color = 0);
		void EndEvent();
		void ScopedEvent(const char* name, uint64 color = 0);

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

		void TransitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel = ~0);
		void TransitionResource(Texture* texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel = ~0);

		void TransitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState);
		void TransitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState);

		void UAVBarrier(Handle<Texture> texture);
		void UAVBarrier(Handle<Buffer> buffer);

		void SubmitResourceBarriers();
		void BindSwapchainRenderTargets();

		void Reset();
		void Execute();
		void Free(uint64 fenceValue);

	private:
		void InstallDrawState();
	};
}

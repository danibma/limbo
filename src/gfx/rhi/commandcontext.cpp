#include "stdafx.h"
#include "commandcontext.h"
#include "resourcemanager.h"
#include "accelerationstructure.h"
#include "commandqueue.h"
#include "device.h"
#include "ringbufferallocator.h"
#include "core/utils.h"
#include "gfx/profiler.h"

namespace limbo::RHI
{
	CommandContext::CommandContext(CommandQueue* queue, ContextType type, ID3D12Device5* device, DescriptorHeap* globalHeap)
		: m_ParentQueue(queue), m_Type(type), m_GlobalHeap(globalHeap)
	{
		m_Allocator = queue->RequestAllocator();
		DX_CHECK(device->CreateCommandList(0, D3DCmdListType(type), m_Allocator, nullptr, IID_PPV_ARGS(m_CommandList.ReleaseAndGetAddressOf())));

		std::string name = std::format("{} Command Context", CmdListTypeToStr(m_Type));
		std::wstring wName;
		Utils::StringConvert(name, wName);
		DX_CHECK(m_CommandList->SetName(wName.c_str()));

		if (m_Type != ContextType::Copy)
		{
			ID3D12DescriptorHeap* heaps[] = { m_GlobalHeap->GetHeap() };
			m_CommandList->SetDescriptorHeaps(ARRAY_LEN(heaps), heaps);
		}
	}

	CommandContext::~CommandContext()
	{
		m_ParentQueue->WaitForIdle();
		Free(0);
	}

	void CommandContext::CopyTextureToTexture(TextureHandle src, TextureHandle dst)
	{
		Texture* srcTexture = RM_GET(src);
		Texture* dstTexture = RM_GET(dst);

		InsertResourceBarrier(srcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
		InsertResourceBarrier(dstTexture, D3D12_RESOURCE_STATE_COPY_DEST);

		SubmitResourceBarriers();
		m_CommandList->CopyResource(dstTexture->Resource.Get(), srcTexture->Resource.Get());
	}

	void CommandContext::CopyTextureToBackBuffer(TextureHandle texture)
	{
		TextureHandle backBufferHandle = Device::Ptr->GetCurrentBackbuffer();

		CopyTextureToTexture(texture, backBufferHandle);

		InsertResourceBarrier(RM_GET(backBufferHandle), D3D12_RESOURCE_STATE_RENDER_TARGET);
		SubmitResourceBarriers();
	}

	void CommandContext::CopyBufferToTexture(BufferHandle src, TextureHandle dst, uint64 dstOffset)
	{
		Texture* dstTexture = RM_GET(dst);
		Buffer* srcBuffer = RM_GET(src);

		CopyBufferToTexture(srcBuffer, dstTexture, dstOffset);
	}

	void CommandContext::CopyBufferToTexture(Buffer* src, Texture* dst, uint64 dstOffset)
	{
		InsertResourceBarrier(dst, D3D12_RESOURCE_STATE_COPY_DEST, ~0);
		SubmitResourceBarriers();

		D3D12_RESOURCE_DESC dstDesc = dst->Resource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[D3D12_REQ_MIP_LEVELS] = {};
		Device::Ptr->GetDevice()->GetCopyableFootprints(&dstDesc, 0, dstDesc.MipLevels, dstOffset, footprints, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {
			.pResource = src->Resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = footprints[0]
		};

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {
			.pResource = dst->Resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = 0
		};
		m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	void CommandContext::CopyBufferToBuffer(BufferHandle src, BufferHandle dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		Buffer* srcBuffer = RM_GET(src);
		Buffer* dstBuffer = RM_GET(dst);

		CopyBufferToBuffer(srcBuffer, dstBuffer, numBytes, srcOffset, dstOffset);
	}

	void CommandContext::CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		InsertResourceBarrier(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
		InsertResourceBarrier(dst, D3D12_RESOURCE_STATE_COPY_DEST);
		SubmitResourceBarriers();

		m_CommandList->CopyBufferRegion(dst->Resource.Get(), dstOffset, src->Resource.Get(), srcOffset, numBytes);
	}

	void CommandContext::ClearRenderTargets(Span<TextureHandle> renderTargets, float4 color)
	{
		for (TextureHandle rt : renderTargets)
		{
			InsertResourceBarrier(rt, D3D12_RESOURCE_STATE_RENDER_TARGET);
			SubmitResourceBarriers();

			Texture* pRenderTarget = RM_GET(rt);
			m_CommandList->ClearRenderTargetView(pRenderTarget->RTVHandle.CpuHandle, &color.x, 0, nullptr);
		}
	}

	void CommandContext::ClearDepthTarget(TextureHandle depthTarget, float depth, uint8 stencil)
	{
		Texture* dt = RM_GET(depthTarget);

		D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
		if (stencil > 0)
			flags |= D3D12_CLEAR_FLAG_STENCIL;

		m_CommandList->ClearDepthStencilView(dt->RTVHandle.CpuHandle, flags, depth, stencil, 0, nullptr);
	}

	void CommandContext::SetVertexBuffer(BufferHandle buffer)
	{
		Buffer* vb = RM_GET(buffer);
		InsertResourceBarrier(vb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = vb->Resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)vb->ByteSize,
			.StrideInBytes = vb->ByteStride
		};
		m_CommandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void CommandContext::SetIndexBuffer(BufferHandle buffer)
	{
		Buffer* ib = RM_GET(buffer);
		InsertResourceBarrier(ib, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		D3D12_INDEX_BUFFER_VIEW ibView = {
			.BufferLocation = ib->Resource->GetGPUVirtualAddress(),
			.SizeInBytes = (uint32)ib->ByteSize,
			.Format = DXGI_FORMAT_R32_UINT
		};

		m_CommandList->IASetIndexBuffer(&ibView);
	}

	void CommandContext::SetVertexBufferView(VertexBufferView view)
	{
		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = view.BufferLocation,
			.SizeInBytes = view.SizeInBytes,
			.StrideInBytes = view.StrideInBytes
		};
		m_CommandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void CommandContext::SetIndexBufferView(IndexBufferView view)
	{
		D3D12_INDEX_BUFFER_VIEW ibView = {
			.BufferLocation = view.BufferLocation,
			.SizeInBytes = view.SizeInBytes,
			.Format = DXGI_FORMAT_R32_UINT
		};

		m_CommandList->IASetIndexBuffer(&ibView);
	}

	void CommandContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
	{
		m_CommandList->IASetPrimitiveTopology(topology);
	}

	void CommandContext::SetViewport(uint32 width, uint32 height, float topLeft, float topRight, float minDepth, float maxDepth)
	{
		D3D12_VIEWPORT viewport = {
			.TopLeftX = topLeft,
			.TopLeftY = topRight,
			.Width = (float)width,
			.Height = (float)height,
			.MinDepth = minDepth,
			.MaxDepth = maxDepth
		};

		D3D12_RECT scissor = {
			.left = (int)topLeft,
			.top = (int)topRight,
			.right = (int)width,
			.bottom = (int)height
		};

		m_CommandList->RSSetViewports(1, &viewport);
		m_CommandList->RSSetScissorRects(1, &scissor);
	}

	void CommandContext::SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthTarget)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE* depthDescriptor = nullptr;
		if (depthTarget.IsValid())
		{
			Texture* pDepthTarget = RM_GET(depthTarget);
			depthDescriptor = &pDepthTarget->RTVHandle.CpuHandle;
		}

		TStaticArray<D3D12_CPU_DESCRIPTOR_HANDLE, MAX_RENDER_TARGETS> renderTargetDescriptors;
		for (uint32 i = 0; i < renderTargets.GetSize(); ++i)
		{
			Texture* pRenderTarget = RM_GET(renderTargets[i]);
			renderTargetDescriptors[i] = pRenderTarget->RTVHandle.CpuHandle;
			InsertResourceBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		SubmitResourceBarriers();
		m_CommandList->OMSetRenderTargets(renderTargets.GetSize(), renderTargetDescriptors.GetData(), false, depthDescriptor);
	}

	void CommandContext::SetPipelineState(PSOHandle pso)
	{
		m_BoundPSO = pso;

		PipelineStateObject* pPso = RM_GET(pso);

		if (!pPso->IsRaytracing())
			m_CommandList->SetPipelineState(pPso->GetPipelineState());
		else
			m_CommandList->SetPipelineState1(pPso->GetStateObject());

		RootSignature* pRS = RM_GET(pPso->GetRootSignature());

		if (!pPso->IsCompute())
			m_CommandList->SetGraphicsRootSignature(pRS->Get());
		else
			m_CommandList->SetComputeRootSignature(pRS->Get());
	}

	void CommandContext::BindTempDescriptorTable(uint32 rootParameter, DescriptorHandle* handles, uint32 count)
	{
		constexpr uint64 MaxBindCount = 16;
		constexpr uint32 DescriptorCopyRanges[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		static_assert(ARRAY_LEN(DescriptorCopyRanges) == MaxBindCount);

		check(count < MaxBindCount);
		check(count > 0);

		DescriptorHandle tempAlloc = m_GlobalHeap->AllocateTemp(count);

		D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors[MaxBindCount];
		for (uint32 i = 0; i < count; ++i)
			srcDescriptors[i] = handles[i].CpuHandle;

		uint32 destRanges[1] = { count };
		Device::Ptr->GetDevice()->CopyDescriptors(1, &tempAlloc.CpuHandle, destRanges, count, srcDescriptors, DescriptorCopyRanges, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		PipelineStateObject* pPso = RM_GET(m_BoundPSO);
		if (!pPso->IsCompute())
			m_CommandList->SetGraphicsRootDescriptorTable(rootParameter, tempAlloc.GPUHandle);
		else
			m_CommandList->SetComputeRootDescriptorTable(rootParameter, tempAlloc.GPUHandle);
	}

	void CommandContext::BindConstants(uint32 rootParameter, uint32 num32bitValues, uint32 offsetIn32bits, const void* data)
	{
		PipelineStateObject* pPso = RM_GET(m_BoundPSO);
		if (!pPso->IsCompute())
			m_CommandList->SetGraphicsRoot32BitConstants(rootParameter, num32bitValues, data, offsetIn32bits);
		else
			m_CommandList->SetComputeRoot32BitConstants(rootParameter, num32bitValues, data, offsetIn32bits);
	}

	void CommandContext::BindTempConstantBuffer(uint32 rootParameter, const void* data, uint64 dataSize)
	{
		RingBufferAllocation allocation;
		RHI::GetTempBufferAllocator()->AllocateTemp(Math::Align(dataSize, 256ull), allocation);
		memcpy(allocation.MappedData, data, dataSize);
		BindRootCBV(rootParameter, allocation.GPUAddress);
	}

	void CommandContext::BindRootSRV(uint32 rootParameter, uint64 gpuVirtualAddress)
	{
		PipelineStateObject* pPso = RM_GET(m_BoundPSO);
		if (!pPso->IsCompute())
			m_CommandList->SetGraphicsRootShaderResourceView(rootParameter, gpuVirtualAddress);
		else
			m_CommandList->SetComputeRootShaderResourceView(rootParameter, gpuVirtualAddress);
	}

	void CommandContext::BindRootCBV(uint32 rootParameter, uint64 gpuVirtualAddress)
	{
		PipelineStateObject* pPso = RM_GET(m_BoundPSO);
		if (!pPso->IsCompute())
			m_CommandList->SetGraphicsRootConstantBufferView(rootParameter, gpuVirtualAddress);
		else
			m_CommandList->SetComputeRootConstantBufferView(rootParameter, gpuVirtualAddress);
	}

	bool CommandContext::IsTransitionAllowed(D3D12_RESOURCE_STATES state)
	{
		constexpr int VALID_COMPUTE_QUEUE_RESOURCE_STATES =
			D3D12_RESOURCE_STATE_COMMON
			| D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			| D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			| D3D12_RESOURCE_STATE_COPY_DEST
			| D3D12_RESOURCE_STATE_COPY_SOURCE
			| D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

		constexpr int VALID_COPY_QUEUE_RESOURCE_STATES =
			D3D12_RESOURCE_STATE_COMMON
			| D3D12_RESOURCE_STATE_COPY_DEST
			| D3D12_RESOURCE_STATE_COPY_SOURCE;

		if (D3DCmdListType(m_Type) == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{
			return (state & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == state;
		}
		else if (D3DCmdListType(m_Type) == D3D12_COMMAND_LIST_TYPE_COPY)
		{
			return (state & VALID_COPY_QUEUE_RESOURCE_STATES) == state;
		}
		return true;
	}

	void CommandContext::Reset()
	{
		m_Allocator = m_ParentQueue->RequestAllocator();
		DX_CHECK(m_CommandList->Reset(m_Allocator, nullptr));
		m_bIsClosed = false;

		if (m_Type != ContextType::Copy)
		{
			ID3D12DescriptorHeap* heaps[] = { m_GlobalHeap->GetHeap() };
			m_CommandList->SetDescriptorHeaps(ARRAY_LEN(heaps), heaps);
		}
	}

	uint64 CommandContext::Execute()
	{
		DX_CHECK(m_CommandList->Close());
		m_bIsClosed = true;
		return m_ParentQueue->ExecuteCommandLists(this);
	}

	void CommandContext::Free(uint64 fenceValue)
	{
		m_ParentQueue->FreeAllocator(m_Allocator, fenceValue);
		m_Allocator = nullptr;

		if (m_bIsClosed)
			Reset();
	}

	void CommandContext::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		SubmitResourceBarriers();
		m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandContext::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance)
	{
		SubmitResourceBarriers();
		m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	void CommandContext::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		SubmitResourceBarriers();
		m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void CommandContext::DispatchRays(const ShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth)
	{
		SubmitResourceBarriers();

		D3D12_DISPATCH_RAYS_DESC desc;
		sbt.Commit(desc, width, height, depth);
		m_CommandList->DispatchRays(&desc);
	}

	void CommandContext::DispatchMesh(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		SubmitResourceBarriers();
		m_CommandList->DispatchMesh(groupCountX, groupCountY, groupCountZ);
	}

	void CommandContext::SubmitResourceBarriers()
	{
		if (m_ResourceBarriers.empty()) return;
		m_CommandList->ResourceBarrier((uint32)m_ResourceBarriers.size(), m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}

	void CommandContext::BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, BufferHandle scratch, BufferHandle result)
	{
		InsertResourceBarrier(RM_GET(scratch), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		SubmitResourceBarriers();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = inputs;
		desc.ScratchAccelerationStructureData = RM_GET(scratch)->Resource->GetGPUVirtualAddress();
		desc.DestAccelerationStructureData = RM_GET(result)->Resource->GetGPUVirtualAddress();
		m_CommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}

	void CommandContext::BeginEvent(const char* name, uint64 color)
	{
		PIXBeginEvent(m_CommandList.Get(), color, name);
	}

	void CommandContext::EndEvent()
	{
		PIXEndEvent(m_CommandList.Get());
	}

	void CommandContext::ScopedEvent(const char* name, uint64 color)
	{
		PIXScopedEvent(m_CommandList.Get(), color, name);
	}

	void CommandContext::BeginProfileEvent(const char* name, uint64 color /*= 0*/)
	{
		BeginEvent(name, color);
		PROFILE_BEGIN(this, name);
	}

	void CommandContext::EndProfileEvent(const char* name)
	{
		PROFILE_END(this, name);
		EndEvent();
	}

	void CommandContext::GenerateMipLevels(TextureHandle texture)
	{
		Device::Ptr->GenerateMipLevels(texture);
	}

	CommandContext* CommandContext::GetCommandContext(ContextType type /*= ContextType::Direct*/)
	{
		return Device::Ptr->GetCommandContext(type);
	}
}

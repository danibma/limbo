#include "stdafx.h"
#include "commandcontext.h"

#include "commandqueue.h"
#include "descriptorheap.h"
#include "gfx/gfx.h"
#include "device.h"

namespace limbo::Gfx
{
	CommandContext::CommandContext(CommandQueue* queue, ContextType type, ID3D12Device5* device, DescriptorHeap* globalHeap)
		: m_ParentQueue(queue), m_Type(type), m_GlobalHeap(globalHeap)
	{
		m_Allocator = queue->RequestAllocator();
		DX_CHECK(device->CreateCommandList(0, D3DCmdListType(type), m_Allocator, nullptr, IID_PPV_ARGS(&m_CommandList)));

		std::string name = std::format("{} Command Context", CmdListTypeToStr(m_Type));
		std::wstring wName;
		Utils::StringConvert(name, wName);
		DX_CHECK(m_CommandList->SetName(wName.c_str()));

		if (m_Type != ContextType::Copy)
		{
			ID3D12DescriptorHeap* heaps[] = { m_GlobalHeap->GetHeap() };
			m_CommandList->SetDescriptorHeaps(_countof(heaps), heaps);
		}
	}

	CommandContext::~CommandContext()
	{
		m_ParentQueue->WaitForIdle();
		Free(0);
	}

	void CommandContext::CopyTextureToTexture(Handle<Texture> src, Handle<Texture> dst)
	{
		Texture* srcTexture = GetTexture(src);
		Texture* dstTexture = GetTexture(dst);

		TransitionResource(srcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
		TransitionResource(dstTexture, D3D12_RESOURCE_STATE_COPY_DEST);

		SubmitResourceBarriers();
		m_CommandList->CopyResource(dstTexture->Resource.Get(), srcTexture->Resource.Get());
	}

	void CommandContext::CopyTextureToBackBuffer(Handle<Texture> texture)
	{
		Handle<Texture> backBufferHandle = Device::Ptr->GetCurrentBackbuffer();

		CopyTextureToTexture(texture, backBufferHandle);

		TransitionResource(backBufferHandle, D3D12_RESOURCE_STATE_RENDER_TARGET);
		SubmitResourceBarriers();
	}

	void CommandContext::CopyBufferToTexture(Handle<Buffer> src, Handle<Texture> dst, uint64 dstOffset)
	{
		Texture* dstTexture = ResourceManager::Ptr->GetTexture(dst);
		FAILIF(!dstTexture);
		Buffer* srcBuffer = ResourceManager::Ptr->GetBuffer(src);
		FAILIF(!srcBuffer);

		CopyBufferToTexture(srcBuffer, dstTexture, dstOffset);
	}

	void CommandContext::CopyBufferToTexture(Buffer* src, Texture* dst, uint64 dstOffset)
	{
		TransitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST, ~0);
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

	void CommandContext::CopyBufferToBuffer(Handle<Buffer> src, Handle<Buffer> dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		Buffer* srcBuffer = ResourceManager::Ptr->GetBuffer(src);
		FAILIF(!srcBuffer);
		Buffer* dstBuffer = ResourceManager::Ptr->GetBuffer(dst);
		FAILIF(!dstBuffer);

		CopyBufferToBuffer(srcBuffer, dstBuffer, numBytes, srcOffset, dstOffset);
	}

	void CommandContext::CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64 numBytes, uint64 srcOffset, uint64 dstOffset)
	{
		TransitionResource(src, D3D12_RESOURCE_STATE_GENERIC_READ);
		TransitionResource(dst, D3D12_RESOURCE_STATE_COPY_DEST);
		SubmitResourceBarriers();

		m_CommandList->CopyBufferRegion(dst->Resource.Get(), dstOffset, src->Resource.Get(), srcOffset, numBytes);
	}

	void CommandContext::BindVertexBuffer(Handle<Buffer> buffer)
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

	void CommandContext::BindIndexBuffer(Handle<Buffer> buffer)
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

	void CommandContext::BindVertexBufferView(VertexBufferView view)
	{
		D3D12_VERTEX_BUFFER_VIEW vbView = {
			.BufferLocation = view.BufferLocation,
			.SizeInBytes = view.SizeInBytes,
			.StrideInBytes = view.StrideInBytes
		};
		m_CommandList->IASetVertexBuffers(0, 1, &vbView);
	}

	void CommandContext::BindIndexBufferView(IndexBufferView view)
	{
		D3D12_INDEX_BUFFER_VIEW ibView = {
			.BufferLocation = view.BufferLocation,
			.SizeInBytes = view.SizeInBytes,
			.Format = DXGI_FORMAT_R32_UINT
		};

		m_CommandList->IASetIndexBuffer(&ibView);
	}

	void CommandContext::BindShader(Handle<Shader> shader)
	{
		m_BoundShader = shader;

		ResourceManager* rm = ResourceManager::Ptr;
		Shader* pBoundShader = rm->GetShader(m_BoundShader);

		if (pBoundShader->Type == ShaderType::Graphics)
		{
			int32 width = GetBackbufferWidth();
			int32 height = GetBackbufferHeight();
			if (pBoundShader->UseSwapchainRT)
			{
				BindSwapchainRenderTargets();
			}
			else
			{
				// first transition the render targets to the correct resource state
				for (uint8 i = 0; i < pBoundShader->RTCount; ++i)
					TransitionResource(pBoundShader->RenderTargets[i].Texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
				if (pBoundShader->DepthTarget.Texture.IsValid())
					TransitionResource(pBoundShader->DepthTarget.Texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				SubmitResourceBarriers();

				D3D12_CPU_DESCRIPTOR_HANDLE* dsvhandle = nullptr;

				if (pBoundShader->DepthTarget.Texture.IsValid())
				{
					Texture* depthBackbuffer = rm->GetTexture(pBoundShader->DepthTarget.Texture);
					FAILIF(!depthBackbuffer);
					dsvhandle = &depthBackbuffer->BasicHandle[0].CpuHandle;
					if (pBoundShader->DepthTarget.LoadRenderPassOp == RenderPassOp::Clear)
						m_CommandList->ClearDepthStencilView(depthBackbuffer->BasicHandle[0].CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
				}

				constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				D3D12_CPU_DESCRIPTOR_HANDLE rtHandles[8];
				for (uint8 i = 0; i < pBoundShader->RTCount; ++i)
				{
					Texture* rt = rm->GetTexture(pBoundShader->RenderTargets[i].Texture);
					FAILIF(!rt);

					if (pBoundShader->RenderTargets[i].LoadRenderPassOp == RenderPassOp::Clear)
						m_CommandList->ClearRenderTargetView(rt->BasicHandle[0].CpuHandle, clearColor, 0, nullptr);

					rtHandles[i] = rt->BasicHandle[0].CpuHandle;
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

	void CommandContext::InstallDrawState()
	{
		SubmitResourceBarriers();

		ResourceManager* rm = ResourceManager::Ptr;
		Shader* shader = rm->GetShader(m_BoundShader);

		shader->SetRootParameters(m_CommandList.Get());

		if (shader->Type == ShaderType::Graphics)
			m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (shader->Type != ShaderType::RayTracing)
		{
			m_CommandList->SetPipelineState(shader->PipelineState.Get());
		}
		else
		{
			m_CommandList->SetPipelineState1(shader->StateObject.Get());
		}
	}

	void CommandContext::BindSwapchainRenderTargets()
	{
		ResourceManager* rm = ResourceManager::Ptr;
		Shader* shader = rm->GetShader(m_BoundShader);
		if (!shader->UseSwapchainRT)
			return;

		Handle<Texture> backBufferHandle = Device::Ptr->GetCurrentBackbuffer();
		Texture* backbuffer = rm->GetTexture(backBufferHandle);
		FAILIF(!backbuffer);

		Handle<Texture> depthBackBufferHandle = Device::Ptr->GetCurrentDepthBackbuffer();
		Texture* depthBackbuffer = rm->GetTexture(depthBackBufferHandle);
		FAILIF(!depthBackbuffer);

		m_CommandList->OMSetRenderTargets(1, &backbuffer->BasicHandle[0].CpuHandle, false, &depthBackbuffer->BasicHandle[0].CpuHandle);

		constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_CommandList->ClearDepthStencilView(depthBackbuffer->BasicHandle[0].CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_CommandList->ClearRenderTargetView(backbuffer->BasicHandle[0].CpuHandle, clearColor, 0, nullptr);
	}

	void CommandContext::Reset()
	{
		m_Allocator = m_ParentQueue->RequestAllocator();
		DX_CHECK(m_CommandList->Reset(m_Allocator, nullptr));
		m_bIsClosed = false;

		if (m_Type != ContextType::Copy)
		{
			ID3D12DescriptorHeap* heaps[] = { m_GlobalHeap->GetHeap() };
			m_CommandList->SetDescriptorHeaps(_countof(heaps), heaps);
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
		InstallDrawState();
		m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandContext::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 baseVertex, uint32 firstInstance)
	{
		InstallDrawState();
		m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
	}

	void CommandContext::DispatchRays(const ShaderBindingTable& sbt, uint32 width, uint32 height, uint32 depth)
	{
		InstallDrawState();

		D3D12_DISPATCH_RAYS_DESC desc;
		sbt.Commit(desc, width, height, depth);
		m_CommandList->DispatchRays(&desc);
	}

	void CommandContext::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		InstallDrawState();
		m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void CommandContext::SubmitResourceBarriers()
	{
		if (m_ResourceBarriers.empty()) return;
		m_CommandList->ResourceBarrier((uint32)m_ResourceBarriers.size(), m_ResourceBarriers.data());
		m_ResourceBarriers.clear();
	}

	void CommandContext::BuildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, Handle<Buffer> scratch, Handle<Buffer> result)
	{
		TransitionResource(scratch, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		SubmitResourceBarriers();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = inputs;
		desc.ScratchAccelerationStructureData = GetBuffer(scratch)->Resource->GetGPUVirtualAddress();
		desc.DestAccelerationStructureData = GetBuffer(result)->Resource->GetGPUVirtualAddress();
		m_CommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}


	void CommandContext::TransitionResource(Buffer* buffer, D3D12_RESOURCE_STATES newState)
	{
		if (buffer->CurrentState == newState) return;
		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(buffer->Resource.Get(), buffer->CurrentState, newState));
		buffer->CurrentState = newState;
	}

	void CommandContext::TransitionResource(Handle<Texture> texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel)
	{
		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t);
		TransitionResource(t, newState, mipLevel);
	}

	void CommandContext::TransitionResource(Handle<Buffer> buffer, D3D12_RESOURCE_STATES newState)
	{
		Buffer* b = ResourceManager::Ptr->GetBuffer(buffer);
		FAILIF(!b);
		TransitionResource(b, newState);
	}

	void CommandContext::TransitionResource(Texture* texture, D3D12_RESOURCE_STATES newState, uint32 mipLevel)
	{
		D3D12_RESOURCE_STATES oldState;
		if (mipLevel == ~0)
		{
			if (texture->CurrentState[0] == newState) return;
			oldState = texture->CurrentState[0];
			for (uint8 i = 0; i < texture->Spec.MipLevels; ++i)
				texture->CurrentState[i] = newState;
		}
		else
		{
			if (texture->CurrentState[mipLevel] == newState) return;
			oldState = texture->CurrentState[mipLevel];
			texture->CurrentState[mipLevel] = newState;
		}

		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(), oldState, newState, mipLevel));
	}

	void CommandContext::UAVBarrier(Handle<Texture> texture)
	{
		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t);
		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(t->Resource.Get()));
	}

	void CommandContext::UAVBarrier(Handle<Buffer> buffer)
	{
		Buffer* b = ResourceManager::Ptr->GetBuffer(buffer);
		FAILIF(!b);
		m_ResourceBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(b->Resource.Get()));
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
}

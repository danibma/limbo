#include "stdafx.h"
#include "descriptorheap.h"

#include <d3d12/d3dx12/d3dx12.h>

namespace limbo::Gfx
{
	DescriptorHeap::DescriptorHeap(ID3D12Device* device, DescriptorHeapType heapType, uint32 numDescriptors, bool bShaderVisible)
		: m_MaxDescriptors(numDescriptors), m_bShaderVisible(bShaderVisible)
	{
		m_CurrentDescriptor = 0;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = D3DDescriptorHeapType(heapType),
			.NumDescriptors = m_MaxDescriptors,
			.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3DDescriptorHeapType(heapType));
	}

	DescriptorHeap::~DescriptorHeap()
	{
	}

	DescriptorHandle DescriptorHeap::AllocateHandle()
	{
		DescriptorHandle handle = {};

		if (m_CurrentDescriptor >= m_MaxDescriptors)
		{
			LB_ERROR("Ran out of descriptor heap handles, need to increase heap size.");
			return handle;
		}
		++m_CurrentDescriptor;

		handle.Index = m_CurrentDescriptor;
		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (m_CurrentDescriptor * m_DescriptorSize);
		if (m_bShaderVisible)
			handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (m_CurrentDescriptor * m_DescriptorSize);

		return handle;
	}

	DescriptorHandle DescriptorHeap::GetHandleByIndex(uint32 index)
	{
		DescriptorHandle handle;

		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (index * m_DescriptorSize);
		handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (index * m_DescriptorSize);

		return handle;
	}
}


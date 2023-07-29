#include "stdafx.h"
#include "descriptorheap.h"

#include <d3d12/d3dx12/d3dx12.h>

namespace limbo::gfx
{
	DescriptorHeap::DescriptorHeap(ID3D12Device* device, DescriptorHeapType heapType, bool bShaderVisible)
		: m_bShaderVisible(bShaderVisible)
	{
		m_currentDescriptor = 0;

		D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = d3dDescriptorHeapType(heapType),
			.NumDescriptors = MAX_DESCRIPTOR_NUM,
			.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

		m_descriptorSize = device->GetDescriptorHandleIncrementSize(d3dDescriptorHeapType(heapType));
	}

	DescriptorHeap::~DescriptorHeap()
	{
	}

	DescriptorHandle DescriptorHeap::allocateHandle()
	{
		DescriptorHandle handle = {};

		if (m_currentDescriptor >= MAX_DESCRIPTOR_NUM)
		{
			LB_ERROR("Ran out of descriptor heap handles, need to increase heap size.");
			return handle;
		}
		++m_currentDescriptor;

		handle.cpuHandle.ptr = m_heap->GetCPUDescriptorHandleForHeapStart().ptr + (m_currentDescriptor * m_descriptorSize);
		if (m_bShaderVisible)
			handle.gpuHandle.ptr = m_heap->GetGPUDescriptorHandleForHeapStart().ptr + (m_currentDescriptor * m_descriptorSize);

		return handle;
	}

	DescriptorHandle DescriptorHeap::getHandleByIndex(uint32 index)
	{
		DescriptorHandle handle;

		handle.cpuHandle.ptr = m_heap->GetCPUDescriptorHandleForHeapStart().ptr + (index * m_descriptorSize);
		handle.gpuHandle.ptr = m_heap->GetGPUDescriptorHandleForHeapStart().ptr + (index * m_descriptorSize);

		return handle;
	}
}


#include "stdafx.h"
#include "descriptorheap.h"

#include "commandqueue.h"
#include "device.h"

namespace limbo::Gfx
{
	DescriptorHeap::DescriptorHeap(ID3D12Device* device, DescriptorHeapType heapType, uint32 numDescriptors, bool bShaderVisible)
		: m_HeapType(heapType), m_bShaderVisible(bShaderVisible), m_MaxDescriptors(numDescriptors)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = D3DDescriptorHeapType(heapType),
			.NumDescriptors = m_MaxDescriptors,
			.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3DDescriptorHeapType(heapType));

		m_FreeDescriptors.resize(m_MaxDescriptors);
		for (uint32 i = 0; i < m_MaxDescriptors; ++i)
			m_FreeDescriptors[i] = i;
	}

	DescriptorHeap::~DescriptorHeap()
	{
	}

	DescriptorHandle DescriptorHeap::AllocateHandle()
	{
		DescriptorHandle handle = {};

		uint32 descriptor = GetNextDescriptor();
		handle.OwnerHeapType = m_HeapType;
		handle.Index = descriptor;
		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);
		if (m_bShaderVisible)
			handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);

		return handle;
	}

	DescriptorHandle DescriptorHeap::AllocateTempHandle()
	{
		DescriptorHandle handle = AllocateHandle();
		m_DeletionQueue.emplace_back(handle.Index, Device::Ptr->GetPresentFence()->GetCurrentValue());
		return handle;
	}

	void DescriptorHeap::FreeHandle(DescriptorHandle& handle)
	{
		if (handle.CpuHandle.ptr == 0) return; // not valid

		m_DeletionQueue.emplace_back(handle.Index, Device::Ptr->GetPresentFence()->GetCurrentValue());
		handle = {};
	}

	DescriptorHandle DescriptorHeap::GetHandleByIndex(uint32 index)
	{
		DescriptorHandle handle;

		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (index * m_DescriptorSize);
		handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (index * m_DescriptorSize);
		handle.Index		 = index;
		handle.OwnerHeapType = m_HeapType;

		return handle;
	}

	uint32 DescriptorHeap::GetNextDescriptor()
	{
		if (m_FreeDescriptors.empty())
		{
			while (!m_DeletionQueue.empty())
			{
				if (!Device::Ptr->GetPresentFence()->IsComplete(m_DeletionQueue.front().second))
					break;
					
				m_FreeDescriptors.emplace_back(m_DeletionQueue.front().first);
				m_DeletionQueue.pop_front();
			}
		}

		if (!m_FreeDescriptors.empty())
		{
			uint32 descriptor = m_FreeDescriptors.front();
			m_FreeDescriptors.pop_front();
			return descriptor;
		}

		LB_ERROR("Ran out of descriptor heap handles, need to increase heap size.");
		return ~0;
	}
}


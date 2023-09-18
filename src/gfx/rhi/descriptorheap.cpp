#include "stdafx.h"
#include "descriptorheap.h"

#include "commandqueue.h"
#include "device.h"

namespace limbo::RHI
{
	DescriptorHeap::DescriptorHeap(ID3D12Device5* device, DescriptorHeapType heapType, uint32 numPersistent, uint32 numTemporary, bool bShaderVisible)
		: m_HeapType(heapType), m_bShaderVisible(bShaderVisible), m_NumPersistent(numPersistent), m_NumTemporary(numTemporary)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = D3DDescriptorHeapType(heapType),
			.NumDescriptors = m_NumPersistent + m_NumTemporary,
			.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};
		DX_CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3DDescriptorHeapType(heapType));

		m_FreePersistents.resize(m_NumPersistent);
		for (uint32 i = 0; i < m_NumPersistent; ++i)
			m_FreePersistents[i] = i;

		m_FreeTemporary.resize(m_NumTemporary);
		for (uint32 i = 0; i < m_NumTemporary; ++i)
			m_FreeTemporary[i] = i + m_NumPersistent;
	}

	DescriptorHeap::~DescriptorHeap()
	{
	}

	DescriptorHandle DescriptorHeap::AllocatePersistent()
	{
		DescriptorHandle handle = {};

		uint32 descriptor = GetNextPersistent();
		handle.OwnerHeapType = m_HeapType;
		handle.Index = descriptor;
		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);
		if (m_bShaderVisible)
			handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);

		return handle;
	}

	void DescriptorHeap::FreePersistent(DescriptorHandle& handle)
	{
		if (handle.CpuHandle.ptr == 0) return; // not valid
		m_FreePersistents.emplace_back(handle.Index);
		handle = {};
	}

	uint32 DescriptorHeap::GetNextPersistent()
	{
		if (m_FreePersistents.empty())
		{
			LB_ERROR("Ran out of persistent descriptor heap handles, need to increase heap size.");
			return ~0;
		}

		uint32 descriptor = m_FreePersistents.front();
		m_FreePersistents.pop_front();
		return descriptor;
	}

	uint32 DescriptorHeap::GetNextTemporary()
	{
		if (m_FreeTemporary.empty())
		{
			while (!m_TempDeletionQueue.empty())
			{
				if (!Device::Ptr->GetPresentFence()->IsComplete(m_TempDeletionQueue.front().second))
					break;

				m_FreeTemporary.emplace_back(m_TempDeletionQueue.front().first);
				m_TempDeletionQueue.pop_front();
			}
		}

		uint32 descriptor = m_FreeTemporary.front();
		m_FreeTemporary.pop_front();
		return descriptor;
	}

	DescriptorHandle DescriptorHeap::AllocateTemp(uint32 count)
	{
		DescriptorHandle handle = {};

		uint32 descriptor = GetNextTemporary();
		handle.OwnerHeapType = m_HeapType;
		handle.Index = descriptor;
		handle.CpuHandle.ptr = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);
		if (m_bShaderVisible)
			handle.GPUHandle.ptr = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr + (descriptor * m_DescriptorSize);

		m_TempDeletionQueue.emplace_back(handle.Index, Device::Ptr->GetPresentFence()->GetCurrentValue());
		return handle;
	}
}


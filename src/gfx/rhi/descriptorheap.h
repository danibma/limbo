#pragma once

#include "core/core.h"
#include "definitions.h"
#include "core/refcountptr.h"

namespace limbo::RHI
{
	enum class DescriptorHeapType : uint8
	{
		SRV,
		RTV,
		DSV,
		UAV,
		CBV
	};

	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
		uint32						Index;

		DescriptorHeapType			OwnerHeapType;
	};

	class DescriptorHeap
	{
		DescriptorHeapType						m_HeapType;

		RefCountPtr<ID3D12DescriptorHeap>			m_Heap;
		bool									m_bShaderVisible;
		uint32									m_DescriptorSize;

		uint32									m_NumPersistent;

		uint32									m_NumTemporary;

		std::deque<uint32>						m_FreePersistents;
		std::deque<uint32>						m_FreeTemporary;

		// Descriptor Index, Fence Value - the descriptors can only be used when the operation corresponding to that fence value was already completed
		std::deque<std::pair<uint32, uint64>>	m_TempDeletionQueue;
	public:
		DescriptorHeap() = delete;
		DescriptorHeap(ID3D12Device5* device, DescriptorHeapType heapType, uint32 numPersistent, uint32 numTemporary, bool bShaderVisible = false);
		~DescriptorHeap();

		DescriptorHeap(DescriptorHeap& heap) = delete;
		DescriptorHeap(DescriptorHeap&& heap) = delete;

		DescriptorHandle AllocatePersistent();
		void FreePersistent(DescriptorHandle& handle);

		DescriptorHandle AllocateTemp(uint32 count = 1);

		ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }

	private:
		uint32 GetNextPersistent();
		uint32 GetNextTemporary();
	};

	inline D3D12_DESCRIPTOR_HEAP_TYPE D3DDescriptorHeapType(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::CBV:
		case DescriptorHeapType::UAV:
		case DescriptorHeapType::SRV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case DescriptorHeapType::RTV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case DescriptorHeapType::DSV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		default: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		}
	}
}

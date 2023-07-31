#pragma once

#include "core/core.h"
#include "definitions.h"

namespace limbo::Gfx
{
	enum class DescriptorHeapType : uint8
	{
		SRV,
		RTV,
		DSV,
		SAMPLERS
	};

	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	};

	class DescriptorHeap
	{
		ComPtr<ID3D12DescriptorHeap>			m_Heap;

		uint32									m_CurrentDescriptor;
		uint32									m_DescriptorSize;

		const uint32							MAX_DESCRIPTOR_NUM = 1000;

		bool									m_bShaderVisible;

	public:
		DescriptorHeap() = default;
		DescriptorHeap(ID3D12Device* device, DescriptorHeapType heapType, bool bShaderVisible = false);
		~DescriptorHeap();

		DescriptorHeap(DescriptorHeap& heap) = delete;
		DescriptorHeap(DescriptorHeap&& heap) = delete;

		DescriptorHandle AllocateHandle();

		DescriptorHandle GetHandleByIndex(uint32 index);

		ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
	};

	inline D3D12_DESCRIPTOR_HEAP_TYPE D3DDescriptorHeapType(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case DescriptorHeapType::SRV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case DescriptorHeapType::RTV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case DescriptorHeapType::DSV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		case DescriptorHeapType::SAMPLERS:
			return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		default: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		}
	}
}

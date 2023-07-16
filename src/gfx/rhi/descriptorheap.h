#pragma once

#include "core.h"
#include "definitions.h"

namespace limbo::gfx
{
	enum class DescriptorHeapType : uint8
	{
		SRV,
		RTV,
		DSV
	};

	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	class DescriptorHeap
	{
		ComPtr<ID3D12DescriptorHeap>			m_heap;

		uint32									m_currentDescriptor;
		uint32									m_descriptorSize;

		const uint32							MAX_DESCRIPTOR_NUM = 1000;

		bool									m_bShaderVisible;

	public:
		DescriptorHeap() = default;
		DescriptorHeap(ID3D12Device* device, DescriptorHeapType heapType, bool bShaderVisible = false);
		~DescriptorHeap();

		DescriptorHeap(DescriptorHeap& heap) = delete;
		DescriptorHeap(DescriptorHeap&& heap) = delete;

		DescriptorHandle allocateHandle();

		DescriptorHandle getHandleByIndex(uint32 index);

		ID3D12DescriptorHeap* getHeap() const { return m_heap.Get(); }
	};

	inline D3D12_DESCRIPTOR_HEAP_TYPE d3dDescriptorHeapType(DescriptorHeapType heapType)
	{
		switch (heapType)
		{
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

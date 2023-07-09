#pragma once

#include "core.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	enum class D3D12DescriptorHeapType : uint8
	{
		SRV,
		RTV,
		DSV
	};

	struct D3D12DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	};

	class D3D12DescriptorHeap
	{
		ComPtr<ID3D12DescriptorHeap>			m_heap;

		uint32									m_currentDescriptor;
		uint32									m_descriptorSize;

		const uint32							MAX_DESCRIPTOR_NUM = 1000;

		bool									m_bShaderVisible;

	public:
		D3D12DescriptorHeap() = default;
		D3D12DescriptorHeap(ID3D12Device* device, D3D12DescriptorHeapType heapType, bool bShaderVisible = false);
		~D3D12DescriptorHeap();

		D3D12DescriptorHeap(D3D12DescriptorHeap& heap) = delete;
		D3D12DescriptorHeap(D3D12DescriptorHeap&& heap) = delete;

		D3D12DescriptorHandle allocateHandle();

		D3D12DescriptorHandle getHandleByIndex(uint32 index);

		ID3D12DescriptorHeap* getHeap() const { return m_heap.Get(); }
	};

	inline D3D12_DESCRIPTOR_HEAP_TYPE d3dDescriptorHeapType(D3D12DescriptorHeapType heapType)
	{
		switch (heapType)
		{
		case D3D12DescriptorHeapType::SRV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case D3D12DescriptorHeapType::RTV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case D3D12DescriptorHeapType::DSV: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		default: 
			return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		}
	}
}

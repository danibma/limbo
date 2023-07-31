#include "stdafx.h"
#include "sampler.h"
#include "device.h"

namespace limbo::Gfx
{
	Sampler::Sampler(const D3D12_SAMPLER_DESC& desc)
	{
		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		Handle = device->AllocateHandle(DescriptorHeapType::SAMPLERS);

		d3ddevice->CreateSampler(&desc, Handle.CpuHandle);
	}
}

#include "stdafx.h"
#include "sampler.h"
#include "device.h"

namespace limbo::gfx
{
	Sampler::Sampler(const D3D12_SAMPLER_DESC& desc)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		handle = device->allocateHandle(DescriptorHeapType::SAMPLERS);

		d3ddevice->CreateSampler(&desc, handle.cpuHandle);
	}
}

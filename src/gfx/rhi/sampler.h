#pragma once

#include "descriptorheap.h"

namespace limbo::gfx
{
	class Sampler
	{
	public:
		DescriptorHandle handle;

	public:
		Sampler() = default;
		Sampler(const D3D12_SAMPLER_DESC& desc);
	};
}


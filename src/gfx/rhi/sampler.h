#pragma once

#include "descriptorheap.h"

namespace limbo::Gfx
{
	class Sampler
	{
	public:
		DescriptorHandle Handle;

	public:
		Sampler() = default;
		Sampler(const D3D12_SAMPLER_DESC& desc);
	};
}


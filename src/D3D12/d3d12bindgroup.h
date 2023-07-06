#pragma once

#include "shader.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12BindGroup : public BindGroup
	{
	public:
		D3D12BindGroup() = default;
		D3D12BindGroup(const BindGroupSpec& spec);
		virtual ~D3D12BindGroup();
	};
}


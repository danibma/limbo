#pragma once

#include "shader.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12Shader final : public Shader
	{
	public:
		D3D12Shader() = default;
		D3D12Shader(const ShaderSpec& spec);

		virtual ~D3D12Shader();
	};
}
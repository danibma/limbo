#pragma once

#include "shader.h"

namespace limbo::rhi
{
	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const ShaderSpec& spec);

		virtual ~VulkanShader();
	};
}
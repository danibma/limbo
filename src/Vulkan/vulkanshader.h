#pragma once

#include "shader.h"

namespace limbo
{
	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const ShaderSpec& spec);

		virtual ~VulkanShader();
	};
}
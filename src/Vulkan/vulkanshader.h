#pragma once

#include <vector>

#include "shader.h"
#include "shadercompiler.h"
#include "vulkandefinitions.h"

namespace limbo::rhi
{
	class VulkanShader final : public Shader
	{
	private:
		VkPipeline				m_pipeline;
		VkPipelineLayout		m_layout;

	public:
		VulkanShader() = default;
		VulkanShader(const ShaderSpec& spec);

		virtual ~VulkanShader();

	private:
		void createComputePipeline(VkDevice device, const ShaderSpec& spec);
		void createGraphicsPipeline(VkDevice device);
	};
}
#pragma once

#include "shader.h"
#include "vulkandefinitions.h"

namespace limbo::rhi
{
	class VulkanShader final : public Shader
	{
	public:
		VkPipeline				pipeline;
		VkPipelineLayout		layout;

	public:
		VulkanShader() = default;
		VulkanShader(const ShaderSpec& spec);

		virtual ~VulkanShader();

	private:
		void createComputePipeline(VkDevice device, const ShaderSpec& spec);
		void createGraphicsPipeline(VkDevice device);
	};
}
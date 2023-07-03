#include "vulkanshader.h"

#include <array>

#include "vulkanbindgroup.h"
#include "vulkandevice.h"
#include "vulkanresourcemanager.h"

namespace limbo::rhi
{
	VulkanShader::VulkanShader(const ShaderSpec& spec)
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkDevice vkDevice = device->getDevice();

		if (spec.type == ShaderType::Compute)
			createComputePipeline(vkDevice, spec);
		else if (spec.type == ShaderType::Graphics)
			createGraphicsPipeline(vkDevice);
	}

	VulkanShader::~VulkanShader()
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		vk::vkDestroyPipeline(device->getDevice(), m_pipeline, nullptr);
		vk::vkDestroyPipelineLayout(device->getDevice(), m_layout, nullptr);
	}

	void VulkanShader::createComputePipeline(VkDevice device, const ShaderSpec& spec)
	{
		// Create shader module
		ShaderResult shaderResult = {};
		FAILIF(!ShaderCompiler::compile(shaderResult, spec.programName, spec.entryPoint, KernelType::Compute, true));

		VkShaderModuleCreateInfo shaderInfo = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shaderResult.size,
			.pCode = static_cast<uint32*>(shaderResult.code)
		};
		VkShaderModule shader;
		VK_CHECK(vk::vkCreateShaderModule(device, &shaderInfo, nullptr, &shader));

		// Set resource bindings
		VulkanResourceManager* rm = ResourceManager::getAs<VulkanResourceManager>();
		const uint32 bindGroupsCount = (uint32)spec.bindGroups.size();
		std::vector<VkDescriptorSetLayout> layouts(bindGroupsCount);
		for (uint32 i = 0; i < bindGroupsCount; ++i)
		{
			VulkanBindGroup* bind = rm->getBindGroup(spec.bindGroups[i]);
			FAILIF(!bind);
			layouts[i] = bind->getSetLayout();
		}

		// Create layout
		VkPipelineLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = bindGroupsCount,
			.pSetLayouts = layouts.data(),
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};
		VK_CHECK(vk::vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_layout));

		// Create pipeline
		VkComputePipelineCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = shader,
				.pName = spec.entryPoint,
			},
			.layout = m_layout,
			.basePipelineHandle = nullptr,
			.basePipelineIndex = -1 
		};

		VK_CHECK(vk::vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &m_pipeline));

		vk::vkDestroyShaderModule(device, shader, nullptr);
	}

	void VulkanShader::createGraphicsPipeline(VkDevice device)
	{
		ensure(false);
	}
}

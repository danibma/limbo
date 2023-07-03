#pragma once

#include "shader.h"
#include "vulkandefinitions.h"

namespace limbo::rhi
{
	class VulkanBindGroup : public BindGroup
	{
	private:
		VkDescriptorSetLayout		m_setLayout;

	public:
		VulkanBindGroup() = default;
		VulkanBindGroup(const BindGroupSpec& spec);
		virtual ~VulkanBindGroup();

		// Vulkan specific
		VkDescriptorSetLayout getSetLayout() { return m_setLayout; }
	};
}


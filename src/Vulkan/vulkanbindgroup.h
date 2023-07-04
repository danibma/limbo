#pragma once

#include "shader.h"
#include "vulkandefinitions.h"

namespace limbo::rhi
{
	class VulkanBindGroup : public BindGroup
	{
		struct Binding
		{
			uint32					binding;
			VkDescriptorType		descriptorType;
			VkDescriptorImageInfo	imageInfo;
			VkDescriptorBufferInfo	bufferInfo;
		};

	public:
		VkDescriptorSetLayout		setLayout;
		VkDescriptorSet				set;
		std::vector<Binding>		bindings;

	public:
		VulkanBindGroup() = default;
		VulkanBindGroup(const BindGroupSpec& spec);
		virtual ~VulkanBindGroup();

		void update(VkDevice device);
	};
}


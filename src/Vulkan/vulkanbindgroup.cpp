#include "vulkanbindgroup.h"
#include "vulkandevice.h"

namespace limbo::rhi
{
	VulkanBindGroup::VulkanBindGroup(const BindGroupSpec& spec)
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkDevice vkDevice = device->getDevice();

		const uint32 buffersCount = (uint32)spec.buffers.size();
		const uint32 texturesCount = (uint32)spec.textures.size();
		const uint32 bindingCount = buffersCount + texturesCount;
		std::vector<VkDescriptorSetLayoutBinding> bindings(bindingCount);

		for (uint32 i = 0; i < buffersCount; ++i)
		{
			bindings[i].binding			= spec.buffers[i].slot;
			bindings[i].descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // #todo: when the buffers are implemented, this has to change
			bindings[i].descriptorCount = 1;
			bindings[i].stageFlags		= VK_SHADER_STAGE_ALL;
		}

		for (uint32 i = buffersCount; i < texturesCount; ++i)
		{
			bindings[i].binding				= spec.textures[i].slot;
			bindings[i].descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; // #todo: when the textures are implemented, this has to change
			bindings[i].descriptorCount		= 1;
			bindings[i].stageFlags			= VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutCreateInfo setLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings = bindings.data()
		};
		VK_CHECK(vk::vkCreateDescriptorSetLayout(vkDevice, &setLayoutInfo, nullptr, &m_setLayout));
	}

	VulkanBindGroup::~VulkanBindGroup()
	{
		vk::vkDestroyDescriptorSetLayout(Device::getAs<VulkanDevice>()->getDevice(), m_setLayout, nullptr);
	}
}

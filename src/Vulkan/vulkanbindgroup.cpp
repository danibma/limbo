#include "vulkanbindgroup.h"
#include "vulkandevice.h"
#include "vulkanresourcemanager.h"
#include "vulkantexture.h"

namespace limbo::rhi
{
	VulkanBindGroup::VulkanBindGroup(const BindGroupSpec& spec)
	{
		VulkanDevice* device = Device::getAs<VulkanDevice>();
		VkDevice vkDevice = device->getDevice();

		VulkanResourceManager* rm = ResourceManager::getAs<VulkanResourceManager>();

		const uint32 buffersCount = (uint32)spec.buffers.size();
		const uint32 texturesCount = (uint32)spec.textures.size();
		const uint32 bindingCount = buffersCount + texturesCount;
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings(bindingCount);
		bindings.resize(bindingCount);

		for (uint32 i = 0; i < buffersCount; ++i)
		{
			setLayoutBindings[i].binding			= spec.buffers[i].slot;
			setLayoutBindings[i].descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // #todo: when the buffers are implemented, this has to change
			setLayoutBindings[i].descriptorCount	= 1;
			setLayoutBindings[i].stageFlags			= VK_SHADER_STAGE_ALL;

			bindings[i] = {
				.binding		= spec.buffers[i].slot,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.imageInfo		= nullptr,
				.bufferInfo		= {
					.buffer = nullptr, // #todo: implement
					.offset = 0,
					.range  = VK_WHOLE_SIZE
				}
			};
		}

		for (uint32 i = buffersCount; i < texturesCount; ++i)
		{
			VulkanTexture* texture = rm->getTexture(spec.textures[i].texture);
			if (!texture) continue;

			setLayoutBindings[i].binding				= spec.textures[i].slot;
			setLayoutBindings[i].descriptorType			= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; // #todo: when the textures are implemented, this has to change
			setLayoutBindings[i].descriptorCount		= 1;
			setLayoutBindings[i].stageFlags				= VK_SHADER_STAGE_ALL;

			bindings[i] = {
				.binding = spec.textures[i].slot,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.imageInfo = {
					.sampler	 = nullptr,
					.imageView	 = texture->imageView,
					.imageLayout = texture->layout
				},
				.bufferInfo = nullptr
			};
		}

		VkDescriptorSetLayoutCreateInfo setLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings = setLayoutBindings.data()
		};
		VK_CHECK(vk::vkCreateDescriptorSetLayout(vkDevice, &setLayoutInfo, nullptr, &setLayout));

		VkDescriptorSetAllocateInfo allocationInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = device->getDescriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &setLayout
		};
		VK_CHECK(vk::vkAllocateDescriptorSets(vkDevice, &allocationInfo, &set));
	}

	VulkanBindGroup::~VulkanBindGroup()
	{
		vk::vkDestroyDescriptorSetLayout(Device::getAs<VulkanDevice>()->getDevice(), setLayout, nullptr);
	}

	void VulkanBindGroup::update(VkDevice device)
	{
		static bool executed = false;
		executed = true;

		const uint32 bindingsCount = (uint32)bindings.size();
		std::vector<VkWriteDescriptorSet> writeDescriptors(bindingsCount);
		for (uint32 i = 0; i < bindingsCount; ++i)
		{
			writeDescriptors[i] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = set,
				.dstBinding = bindings[i].binding,
				.descriptorCount = 1,
				.descriptorType = bindings[i].descriptorType,
				.pImageInfo = &bindings[i].imageInfo,
				.pBufferInfo = &bindings[i].bufferInfo,
				.pTexelBufferView = nullptr
			};
		}

		vk::vkUpdateDescriptorSets(device, bindingsCount, writeDescriptors.data(), 0, nullptr);
	}
}

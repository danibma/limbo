#include "vulkanresourcemanager.h"

namespace limbo
{
	VulkanResourceManager::~VulkanResourceManager()
	{
	}

	Handle<Buffer> VulkanResourceManager::createBuffer(const BufferSpec& spec)
	{
		VulkanBuffer* vkBuffer = new VulkanBuffer(spec);
		return m_buffers.allocateHandle(vkBuffer);
	}

}

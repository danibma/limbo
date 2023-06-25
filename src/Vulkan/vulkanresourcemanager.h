#pragma once

#include "resourcemanager.h"
#include "vulkanbuffer.h"

namespace limbo
{
	class VulkanResourceManager final : public ResourceManager
	{
	public:
		VulkanResourceManager() = default;
		virtual ~VulkanResourceManager();

		virtual Handle<Buffer> createBuffer(const BufferSpec& spec);

	private:
		Pool<VulkanBuffer, Buffer> m_buffers;
	};
}

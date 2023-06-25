#pragma once

#include "resourcemanager.h"

namespace limbo
{
	class VulkanResourceManager final : public ResourceManager
	{
	public:
		VulkanResourceManager();
		virtual ~VulkanResourceManager();
	};
}

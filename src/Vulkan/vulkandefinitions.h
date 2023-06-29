#pragma once

#include "vulkanloader.h"
#include "core.h"

namespace limbo::internal
{
	void vkHandleError(int error)
	{
		LB_ERROR("Vulkan error code %d", error);
	}
}
#define VK_CHECK(expression) { VkResult result = expression; if (result != VK_SUCCESS) limbo::internal::vkHandleError(result); }
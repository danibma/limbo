#pragma once

#include "vulkanloader.h"
#include "core.h"

namespace limbo
{
	namespace internal
	{
		void vkHandleError(int error)
		{
			LB_ERROR("Vulkan error code %d", error);
		}
	}

#if LIMBO_DEBUG
	inline VkBool32 vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LB_LOG(pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LB_LOG(pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LB_LOG(pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LB_ERROR(pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
#endif
}
#define VK_CHECK(expression) { VkResult result = expression; if (result != VK_SUCCESS) limbo::internal::vkHandleError(result); }
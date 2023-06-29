#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define ENUM_VK_ENTRYPOINTS_BASE(EnumMacro) \
	EnumMacro(PFN_vkGetInstanceProcAddr, vkGetInstanceProcAddr) \
	EnumMacro(PFN_vkCreateInstance, vkCreateInstance)

#define ENUM_VK_ENTRYPOINTS_ALL(EnumMacro) \
	ENUM_VK_ENTRYPOINTS_BASE(EnumMacro)

#define DECLARE_VK_ENTRYPOINTS(Type, Func) inline Type Func;

namespace limbo::vk
{
	ENUM_VK_ENTRYPOINTS_ALL(DECLARE_VK_ENTRYPOINTS)
}
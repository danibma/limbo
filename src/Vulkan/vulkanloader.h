#pragma once

#include "core.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#if LIMBO_WINDOWS
	#include <vulkan/vulkan_win32.h>
#elif LIMBO_LINUX
	#include <X11/Xlib.h>
	#include "vulkan_xlib.h"
#endif

#define ENUM_VK_ENTRYPOINTS_BASE(EnumMacro) \
	EnumMacro(PFN_vkGetInstanceProcAddr, vkGetInstanceProcAddr) \
	EnumMacro(PFN_vkCreateInstance, vkCreateInstance) \
	EnumMacro(PFN_vkGetPhysicalDeviceProperties, vkGetPhysicalDeviceProperties) \
	EnumMacro(PFN_vkCreateDevice, vkCreateDevice) \
	EnumMacro(PFN_vkGetPhysicalDeviceFeatures2, vkGetPhysicalDeviceFeatures2) \
	EnumMacro(PFN_vkGetPhysicalDeviceQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties) \

#define ENUM_VK_ENTRYPOINTS_INSTANCE(EnumMacro) \
	EnumMacro(PFN_vkEnumeratePhysicalDevices, vkEnumeratePhysicalDevices) \
	EnumMacro(PFN_vkCreateDebugUtilsMessengerEXT, vkCreateDebugUtilsMessengerEXT) \
	EnumMacro(PFN_vkDestroyInstance, vkDestroyInstance) \
	EnumMacro(PFN_vkDestroyDevice, vkDestroyDevice) \
	EnumMacro(PFN_vkDestroyDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT) \
	EnumMacro(PFN_vkDestroySurfaceKHR, vkDestroySurfaceKHR) \
	EnumMacro(PFN_vkCreateSwapchainKHR, vkCreateSwapchainKHR) \
	EnumMacro(PFN_vkDestroySwapchainKHR, vkDestroySwapchainKHR) \
	EnumMacro(PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR, vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	EnumMacro(PFN_vkGetPhysicalDeviceSurfaceFormatsKHR, vkGetPhysicalDeviceSurfaceFormatsKHR) \
	EnumMacro(PFN_vkGetSwapchainImagesKHR, vkGetSwapchainImagesKHR)

#if LIMBO_WINDOWS
	#define ENUM_VK_ENTRYPOINTS_PLATFORM_INSTANCE(EnumMacro) \
		EnumMacro(PFN_vkCreateWin32SurfaceKHR, vkCreateWin32SurfaceKHR) \

#elif LIMBO_LINUX
#error Not implemented yet
#endif

#define ENUM_VK_ENTRYPOINTS_ALL(EnumMacro) \
	ENUM_VK_ENTRYPOINTS_BASE(EnumMacro) \
	ENUM_VK_ENTRYPOINTS_INSTANCE(EnumMacro) \
	ENUM_VK_ENTRYPOINTS_PLATFORM_INSTANCE(EnumMacro)

#define DECLARE_VK_ENTRYPOINTS(Type, Func) inline Type Func;

namespace limbo::vk
{
	ENUM_VK_ENTRYPOINTS_ALL(DECLARE_VK_ENTRYPOINTS)
}

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
	EnumMacro(PFN_vkGetPhysicalDeviceFeatures2, vkGetPhysicalDeviceFeatures2) \

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
	EnumMacro(PFN_vkGetSwapchainImagesKHR, vkGetSwapchainImagesKHR) \
	EnumMacro(PFN_vkGetPhysicalDeviceFeatures, vkGetPhysicalDeviceFeatures) \
	EnumMacro(PFN_vkGetPhysicalDeviceFormatProperties, vkGetPhysicalDeviceFormatProperties) \
	EnumMacro(PFN_vkGetPhysicalDeviceImageFormatProperties, vkGetPhysicalDeviceImageFormatProperties) \
	EnumMacro(PFN_vkGetPhysicalDeviceProperties, vkGetPhysicalDeviceProperties) \
	EnumMacro(PFN_vkGetPhysicalDeviceQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties) \
	EnumMacro(PFN_vkGetPhysicalDeviceMemoryProperties, vkGetPhysicalDeviceMemoryProperties) \
	EnumMacro(PFN_vkCreateDevice, vkCreateDevice) \
	EnumMacro(PFN_vkEnumerateDeviceExtensionProperties, vkEnumerateDeviceExtensionProperties) \
	EnumMacro(PFN_vkEnumerateDeviceLayerProperties, vkEnumerateDeviceLayerProperties) \
	EnumMacro(PFN_vkGetDeviceQueue, vkGetDeviceQueue) \
	EnumMacro(PFN_vkQueueSubmit, vkQueueSubmit) \
	EnumMacro(PFN_vkQueueWaitIdle, vkQueueWaitIdle) \
	EnumMacro(PFN_vkDeviceWaitIdle, vkDeviceWaitIdle) \
	EnumMacro(PFN_vkAllocateMemory, vkAllocateMemory) \
	EnumMacro(PFN_vkFreeMemory, vkFreeMemory) \
	EnumMacro(PFN_vkMapMemory, vkMapMemory) \
	EnumMacro(PFN_vkUnmapMemory, vkUnmapMemory) \
	EnumMacro(PFN_vkFlushMappedMemoryRanges, vkFlushMappedMemoryRanges) \
	EnumMacro(PFN_vkInvalidateMappedMemoryRanges, vkInvalidateMappedMemoryRanges) \
	EnumMacro(PFN_vkGetDeviceMemoryCommitment, vkGetDeviceMemoryCommitment) \
	EnumMacro(PFN_vkBindBufferMemory, vkBindBufferMemory) \
	EnumMacro(PFN_vkBindImageMemory, vkBindImageMemory) \
	EnumMacro(PFN_vkGetBufferMemoryRequirements, vkGetBufferMemoryRequirements) \
	EnumMacro(PFN_vkGetImageMemoryRequirements, vkGetImageMemoryRequirements) \
	EnumMacro(PFN_vkGetImageSparseMemoryRequirements, vkGetImageSparseMemoryRequirements) \
	EnumMacro(PFN_vkQueueBindSparse, vkQueueBindSparse) \
	EnumMacro(PFN_vkCreateFence, vkCreateFence) \
	EnumMacro(PFN_vkDestroyFence, vkDestroyFence) \
	EnumMacro(PFN_vkResetFences, vkResetFences) \
	EnumMacro(PFN_vkGetFenceStatus, vkGetFenceStatus) \
	EnumMacro(PFN_vkWaitForFences, vkWaitForFences) \
	EnumMacro(PFN_vkCreateSemaphore, vkCreateSemaphore) \
	EnumMacro(PFN_vkDestroySemaphore, vkDestroySemaphore) \
	EnumMacro(PFN_vkCreateEvent, vkCreateEvent) \
	EnumMacro(PFN_vkDestroyEvent, vkDestroyEvent) \
	EnumMacro(PFN_vkGetEventStatus, vkGetEventStatus) \
	EnumMacro(PFN_vkSetEvent, vkSetEvent) \
	EnumMacro(PFN_vkResetEvent, vkResetEvent) \
	EnumMacro(PFN_vkCreateQueryPool, vkCreateQueryPool) \
	EnumMacro(PFN_vkDestroyQueryPool, vkDestroyQueryPool) \
	EnumMacro(PFN_vkGetQueryPoolResults, vkGetQueryPoolResults) \
	EnumMacro(PFN_vkCreateBuffer, vkCreateBuffer) \
	EnumMacro(PFN_vkDestroyBuffer, vkDestroyBuffer) \
	EnumMacro(PFN_vkCreateBufferView, vkCreateBufferView) \
	EnumMacro(PFN_vkDestroyBufferView, vkDestroyBufferView) \
	EnumMacro(PFN_vkCreateImage, vkCreateImage) \
	EnumMacro(PFN_vkDestroyImage, vkDestroyImage) \
	EnumMacro(PFN_vkGetImageSubresourceLayout, vkGetImageSubresourceLayout) \
	EnumMacro(PFN_vkCreateImageView, vkCreateImageView) \
	EnumMacro(PFN_vkDestroyImageView, vkDestroyImageView) \
	EnumMacro(PFN_vkCreateShaderModule, vkCreateShaderModule) \
	EnumMacro(PFN_vkDestroyShaderModule, vkDestroyShaderModule) \
	EnumMacro(PFN_vkCreatePipelineCache, vkCreatePipelineCache) \
	EnumMacro(PFN_vkDestroyPipelineCache, vkDestroyPipelineCache) \
	EnumMacro(PFN_vkGetPipelineCacheData, vkGetPipelineCacheData) \
	EnumMacro(PFN_vkMergePipelineCaches, vkMergePipelineCaches) \
	EnumMacro(PFN_vkCreateGraphicsPipelines, vkCreateGraphicsPipelines) \
	EnumMacro(PFN_vkCreateComputePipelines, vkCreateComputePipelines) \
	EnumMacro(PFN_vkDestroyPipeline, vkDestroyPipeline) \
	EnumMacro(PFN_vkCreatePipelineLayout, vkCreatePipelineLayout) \
	EnumMacro(PFN_vkDestroyPipelineLayout, vkDestroyPipelineLayout) \
	EnumMacro(PFN_vkCreateSampler, vkCreateSampler) \
	EnumMacro(PFN_vkDestroySampler, vkDestroySampler) \
	EnumMacro(PFN_vkCreateDescriptorSetLayout, vkCreateDescriptorSetLayout) \
	EnumMacro(PFN_vkDestroyDescriptorSetLayout, vkDestroyDescriptorSetLayout) \
	EnumMacro(PFN_vkCreateDescriptorPool, vkCreateDescriptorPool) \
	EnumMacro(PFN_vkDestroyDescriptorPool, vkDestroyDescriptorPool) \
	EnumMacro(PFN_vkResetDescriptorPool, vkResetDescriptorPool) \
	EnumMacro(PFN_vkAllocateDescriptorSets, vkAllocateDescriptorSets) \
	EnumMacro(PFN_vkFreeDescriptorSets, vkFreeDescriptorSets) \
	EnumMacro(PFN_vkUpdateDescriptorSets, vkUpdateDescriptorSets) \
	EnumMacro(PFN_vkCreateFramebuffer, vkCreateFramebuffer) \
	EnumMacro(PFN_vkDestroyFramebuffer, vkDestroyFramebuffer) \
	EnumMacro(PFN_vkCreateRenderPass, vkCreateRenderPass) \
	EnumMacro(PFN_vkDestroyRenderPass, vkDestroyRenderPass) \
	EnumMacro(PFN_vkGetRenderAreaGranularity, vkGetRenderAreaGranularity) \
	EnumMacro(PFN_vkCreateCommandPool, vkCreateCommandPool) \
	EnumMacro(PFN_vkDestroyCommandPool, vkDestroyCommandPool) \
	EnumMacro(PFN_vkResetCommandPool, vkResetCommandPool) \
	EnumMacro(PFN_vkAllocateCommandBuffers, vkAllocateCommandBuffers) \
	EnumMacro(PFN_vkFreeCommandBuffers, vkFreeCommandBuffers) \
	EnumMacro(PFN_vkBeginCommandBuffer, vkBeginCommandBuffer) \
	EnumMacro(PFN_vkEndCommandBuffer, vkEndCommandBuffer) \
	EnumMacro(PFN_vkResetCommandBuffer, vkResetCommandBuffer) \
	EnumMacro(PFN_vkCmdBindPipeline, vkCmdBindPipeline) \
	EnumMacro(PFN_vkCmdSetViewport, vkCmdSetViewport) \
	EnumMacro(PFN_vkCmdSetScissor, vkCmdSetScissor) \
	EnumMacro(PFN_vkCmdSetLineWidth, vkCmdSetLineWidth) \
	EnumMacro(PFN_vkCmdSetDepthBias, vkCmdSetDepthBias) \
	EnumMacro(PFN_vkCmdSetBlendConstants, vkCmdSetBlendConstants) \
	EnumMacro(PFN_vkCmdSetDepthBounds, vkCmdSetDepthBounds) \
	EnumMacro(PFN_vkCmdSetStencilCompareMask, vkCmdSetStencilCompareMask) \
	EnumMacro(PFN_vkCmdSetStencilWriteMask, vkCmdSetStencilWriteMask) \
	EnumMacro(PFN_vkCmdSetStencilReference, vkCmdSetStencilReference) \
	EnumMacro(PFN_vkCmdBindDescriptorSets, vkCmdBindDescriptorSets) \
	EnumMacro(PFN_vkCmdBindIndexBuffer, vkCmdBindIndexBuffer) \
	EnumMacro(PFN_vkCmdBindVertexBuffers, vkCmdBindVertexBuffers) \
	EnumMacro(PFN_vkCmdDraw, vkCmdDraw) \
	EnumMacro(PFN_vkCmdDrawIndexed, vkCmdDrawIndexed) \
	EnumMacro(PFN_vkCmdDrawIndirect, vkCmdDrawIndirect) \
	EnumMacro(PFN_vkCmdDrawIndexedIndirect, vkCmdDrawIndexedIndirect) \
	EnumMacro(PFN_vkCmdDispatch, vkCmdDispatch) \
	EnumMacro(PFN_vkCmdDispatchIndirect, vkCmdDispatchIndirect) \
	EnumMacro(PFN_vkCmdCopyBuffer, vkCmdCopyBuffer) \
	EnumMacro(PFN_vkCmdCopyImage, vkCmdCopyImage) \
	EnumMacro(PFN_vkCmdBlitImage, vkCmdBlitImage) \
	EnumMacro(PFN_vkCmdCopyBufferToImage, vkCmdCopyBufferToImage) \
	EnumMacro(PFN_vkCmdCopyImageToBuffer, vkCmdCopyImageToBuffer) \
	EnumMacro(PFN_vkCmdUpdateBuffer, vkCmdUpdateBuffer) \
	EnumMacro(PFN_vkCmdFillBuffer, vkCmdFillBuffer) \
	EnumMacro(PFN_vkCmdClearColorImage, vkCmdClearColorImage) \
	EnumMacro(PFN_vkCmdClearDepthStencilImage, vkCmdClearDepthStencilImage) \
	EnumMacro(PFN_vkCmdClearAttachments, vkCmdClearAttachments) \
	EnumMacro(PFN_vkCmdResolveImage, vkCmdResolveImage) \
	EnumMacro(PFN_vkCmdSetEvent, vkCmdSetEvent) \
	EnumMacro(PFN_vkCmdResetEvent, vkCmdResetEvent) \
	EnumMacro(PFN_vkCmdWaitEvents, vkCmdWaitEvents) \
	EnumMacro(PFN_vkCmdPipelineBarrier, vkCmdPipelineBarrier) \
	EnumMacro(PFN_vkCmdBeginQuery, vkCmdBeginQuery) \
	EnumMacro(PFN_vkCmdEndQuery, vkCmdEndQuery) \
	EnumMacro(PFN_vkCmdResetQueryPool, vkCmdResetQueryPool) \
	EnumMacro(PFN_vkCmdWriteTimestamp, vkCmdWriteTimestamp) \
	EnumMacro(PFN_vkCmdCopyQueryPoolResults, vkCmdCopyQueryPoolResults) \
	EnumMacro(PFN_vkCmdPushConstants, vkCmdPushConstants) \
	EnumMacro(PFN_vkCmdBeginRenderPass, vkCmdBeginRenderPass) \
	EnumMacro(PFN_vkCmdNextSubpass, vkCmdNextSubpass) \
	EnumMacro(PFN_vkCmdEndRenderPass, vkCmdEndRenderPass) \
	EnumMacro(PFN_vkCmdExecuteCommands, vkCmdExecuteCommands) \
	EnumMacro(PFN_vkAcquireNextImageKHR, vkAcquireNextImageKHR) \
	EnumMacro(PFN_vkQueuePresentKHR, vkQueuePresentKHR) \
	EnumMacro(PFN_vkCmdPipelineBarrier2, vkCmdPipelineBarrier2) \
	EnumMacro(PFN_vkCmdBeginRendering, vkCmdBeginRendering) \
	EnumMacro(PFN_vkCmdEndRendering, vkCmdEndRendering)

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

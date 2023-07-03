#pragma once

#include "vulkanloader.h"
#include "core.h"
#include "definitions.h"

namespace limbo::rhi
{
	inline VkImageType vkImageType(TextureType type)
	{
		switch (type)
		{
		case TextureType::Texture1D:
			return VK_IMAGE_TYPE_1D;
		case TextureType::Texture2D:
			return VK_IMAGE_TYPE_2D;
		case TextureType::Texture3D: 
			return VK_IMAGE_TYPE_3D;
		default: 
			return VK_IMAGE_TYPE_MAX_ENUM;
		}
	}

	inline VkFormat vkFormat(Format format)
	{
		switch (format)
		{
		case Format::R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case Format::R8_UINT:
			return VK_FORMAT_R8_UINT;
		case Format::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case Format::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case Format::R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case Format::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case Format::R32_SFLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case Format::RG8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case Format::RG16_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case Format::RG32_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case Format::B10GR11_UFLOAT_PACK32:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case Format::RGB32_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case Format::RGBA8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Format::A2RGB10_UNORM_PACK32:
			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case Format::RGBA16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case Format::RGBA16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case Format::RGBA16_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::RGBA32_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::D16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case Format::D32_SFLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case Format::D32_SFLOAT_S8_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Format::BC7_UNORM_BLOCK:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case Format::ASTC_4x4_UNORM_BLOCK:
			return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
		case Format::BGRA8_UNORM: 
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Format::MAX: 
		default:
			return VK_FORMAT_MAX_ENUM;
		}
	}

	namespace internal
	{
		inline void vkHandleError(int error)
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
#define VK_CHECK(expression) { VkResult result = expression; if (result != VK_SUCCESS) limbo::rhi::internal::vkHandleError(result); }
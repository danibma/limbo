#pragma once

#include "device.h"

namespace limbo
{
	class VulkanDevice final : public Device
	{
	public:
		VulkanDevice();
		virtual ~VulkanDevice();
	};
}
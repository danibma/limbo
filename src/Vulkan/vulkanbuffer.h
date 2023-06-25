#pragma once

#include "resources.h"

namespace limbo
{
	class VulkanBuffer : public Buffer
	{
	public:
		VulkanBuffer(BufferSpec spec);
	};
}